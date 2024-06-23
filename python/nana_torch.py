import torch

from torch.nn import functional as F

from training import DataProvider, Death, State

def parameter(*args, **kwargs):
    kwargs['requires_grad'] = True
    return torch.empty(*args, **kwargs)

class NN:
    """
    Base class.
    """
    def __init__(self):
        self.weights = []
        self.biases = []

    def load_weights(self, weights, biases):
        self.weights = weights
        self.biases = biases
    
    def initialise(self):
        for weight in self.weights:
            torch.nn.init.normal_(weight, mean=0.0, std=1.0)
            weight.requires_grad = True
        for bias in self.biases:
            torch.nn.init.zeros_(bias)
            bias.requires_grad = True


class AttackEmbedding:
    """
    vector embedding for attacks
    integer (between 0 and 32) -> 2-vector
    """
    def __init__(self):
        self.weights = [
            parameter(2, 32)
        ] 

    def __call__(self, attack: torch.Tensor):
        attack = torch.clip(attack, 0, 32)
        return F.embedding(attack, self.weights[0])


class StateEncoder(NN):
    """
    10x20 board -> 64 state vector
    """

    # constructor
    def __init__(self):
        self.weights = [
            parameter(32, 1, 3, 3), 
            parameter(32, 32, 2, 2), 
            parameter(64, 32, 2, 2),
            parameter(128, 64, 2, 2),
            parameter(64, 1536)
        ]

        self.biases = [
            parameter(32),
            parameter(32),
            parameter(64),
            parameter(128),
            parameter(64)
        ]

    def initialise(self):
        for weight in self.weights[:-1]:
            torch.nn.init.kaiming_normal_(weight, mode='fan_in', nonlinearity='relu')

        torch.nn.init.normal_(self.weights[-1], mean=0.0, std=1.0)

        for bias in self.biases:
            torch.nn.init.zeros_(bias)
            bias.requires_grad = True

    def __call__(self, board):
        weights, biases = self.weights, self.biases

        # 32 channels, 3x3 kernels
        x = F.conv2d(board, weights[0], bias=biases[0], stride=1, padding=0)
        x = F.relu(x)

        # 32 channels, 3x3 kernels
        x = F.conv2d(x, weights[1], bias=biases[1], stride=1, padding=0)
        x = F.relu(x)

        # average pooling
        x = F.avg_pool2d(x, (1,2))

        x = F.batch_norm(x, None, None, weight=None, bias=None, training=True)

        # 32 channels, 3x3 kernels
        x = F.conv2d(x, weights[2], bias=biases[2], stride=1, padding=0)
        x = F.relu(x)

        # average pooling
        x = F.avg_pool2d(x, (2,1))

        x = F.batch_norm(x, None, None, weight=None, bias=None, training=True)

        # 32 channels, 3x3 kernels
        x = F.conv2d(x, weights[3], bias=biases[3], stride=1, padding=0)
        x = F.relu(x)
        
        # average pooling
        x = F.avg_pool2d(x, (1,2))
        
        x = F.batch_norm(x, None, None, weight=None, bias=None, training=True)

        # flatten 
        v = torch.flatten(x, start_dim=1, end_dim=-1)

        # dense
        v = F.linear(v, weights[4], bias=biases[4])
        v = F.relu(v)

        return v


class DeathPredictor(NN):
    """
    64 state vector -> 2 categorical
    """
    def __init__(self):

        super().__init__()

        self.weights = [
            parameter(2, 64),
        ]

        self.biases = [
            parameter(2)
        ]
        

    def __call__(self, v: torch.Tensor):
        
        x = F.linear(v, self.weights[0], bias=self.biases[0])
        x = F.relu(x)
        x = F.softmax(x, dim=-1)

        return x


class StatePredictor(NN):
    """
    64 state vector, attack 2 vector -> 64 vector
    """
    def __init__():
        self.weights = [
            parameter(128, 66),
            parameter(64, 128)
        ]

        self.biases = [
            parameter(128),
            parameter(64)
        ]

    def __call__(v: torch.Tensor, atk: torch.Tensor):

        x = F.concat(v, atk)
        x = F.linear(x, self.weights[0], biases=self.biases[0])
        x = F.relu(x)
        x = F.linear(x, self.weights[1], biases=self.biases[1])
        x = F.relu(x)

        return x


class AttackPredictor(NN):
    """
    64 vector -> attack probabilities 16-vector
    """
    def __init__():
        self.weights = [
            parameter(32, 64),
            parameter(16, 32)
        ]

        self.biases = [
            parameter(32),
            parameter(16)
        ]

    def __call__(v: torch.Tensor):
        x = F.linear(v, self.weights[0], bias=self.biases[0])
        x = F.relu(x)
        x = F.linear(v, self.weights[1], bias=self.biases[1])
        x = F.relu(x)

        x = F.softmax(x, dim=-1)

        return x


class Dataset(torch.utils.data.Dataset):
    def __init__(self, provider: DataProvider):

        self.data: list[tuple[Death, State, State]] = provider.get_games_data_set()

        for idx in range(len(self.data) // 10):
            board1 = torch.from_numpy(self.data[idx][1].board).to(dtype=torch.float32).unsqueeze(0)
            board2 = torch.from_numpy(self.data[idx][2].board).to(dtype=torch.float32).unsqueeze(0)
            death = torch.tensor(self.data[idx][0].death) - 1
            death = torch.clip(death, 0, 1)
            death = F.one_hot(death, 2).to(dtype=torch.float32)

            self.data[idx] = (death, board1, board2)

    def __len__(self):
        return len(self.data) // 10

    def __getitem__(self, idx):
        sample = (
            self.data[idx][1], # board
            self.data[idx][0], # death (one-hot)
        )
        return sample


def train():
    encoder = StateEncoder()
    encoder.initialise()

    predictor = DeathPredictor()
    predictor.initialise()

    batch_size = 256
    epochs = 200

    dataset = Dataset(DataProvider("data.bin"))

    dataloader = torch.utils.data.DataLoader(
        dataset, 
        batch_size=batch_size,
        shuffle=True, 
        num_workers=0
    )

    optimizer = torch.optim.Adam(
        [*encoder.weights, *encoder.biases, *predictor.weights, *predictor.biases],
        lr=0.0001
    )

    for epoch_i in range(epochs):
        epoch_loss = 0

        for batch_idx, data in enumerate(dataloader):
            inputs, targets = data

            optimizer.zero_grad()

            batch_loss = torch.tensor([0.0], requires_grad=True)

            Vs = encoder(inputs)
            outputs = predictor(Vs)

            batch_loss = torch.add(batch_loss, F.cross_entropy(outputs, targets))

            epoch_loss += batch_loss

            batch_loss.backward()

            optimizer.step()

        print("epoch loss: ", float(epoch_loss))

if __name__ == "__main__":
    train()