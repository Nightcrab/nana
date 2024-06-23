import torch

from torch.nn import functional as F

from training import DataProvider, Death, State

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
        for bias in self.biases:
            torch.nn.init.zeros_(bias)


class AttackEmbedding:
    def __init__(weights):
        self.weight = weights[0]

    def __call__(attack: int):
        return F.embedding(attack, self.weight)

class StateEncoder(NN):
    """
    10x20 board -> 64 state vector
    """

    # constructor
    def __init__(self):
        self.weights = [
            torch.empty(32, 1, 3, 3), 
            torch.empty(32, 32, 3, 3), 
            torch.empty(32, 32, 3, 3)
        ]

        self.biases = [
            torch.empty(32),
            torch.empty(32),
            torch.empty(32)
        ]

    def initialise(self):
        for weight in self.weights:
            torch.nn.init.kaiming_uniform_(weight, mode='fan_in', nonlinearity='relu')
        for bias in self.biases:
            torch.nn.init.zeros_(bias)

    def __call__(self, board: torch.Tensor):
        # 32 channels, 3x3 kernels
        x = F.conv2d(board, self.weights[0], bias=self.biases[0], stride=1, padding=0)
        x = F.relu(x)
        
        # 32 channels, 3x3 kernels
        x = F.conv2d(x, self.weights[1], bias=self.biases[1], stride=1, padding=0)
        x = F.relu(x)

        # 32 channels, 3x3 kernels
        x = F.conv2d(x, self.weights[2], bias=self.biases[2], stride=1, padding=0)
        x = F.relu(x)
        
        # average pooling
        x = F.avg_pool2d(x, (2,2))

        # flatten 
        v = torch.flatten(x, start_dim=1, end_dim=-1)

        return v


class DeathPredictor(NN):
    """
    64 state vector -> 2 categorical
    """
    def __init__(self):

        super().__init__()

        self.weights = [
            torch.empty(2, 832),
        ]

        self.biases = [
            torch.empty(2)
        ]
        

    def __call__(self, v: torch.Tensor):
        
        x = F.linear(v, self.weights[0], bias=self.biases[0])
        x = F.relu(x)
        x = F.softmax(x, dim=-1)

        return x


class StatePredictor:
    """
    64 state vector, attack integer -> 64 vector
    """
    def __init__():
        pass

    def __call__(vector: torch.Tensor, attack: torch.Tensor):
        pass


class AttackPredictor:
    """
    64 vector -> attack integer
    """
    def __init__():
        pass

    def __call__(vector: torch.Tensor):
        pass


class Dataset(torch.utils.data.Dataset):
    def __init__(self, provider: DataProvider):
        self.data: list[tuple[Death, State, State]] = provider.get_games_data_set()

    def __len__(self):
        return len(self.data)

    def __getitem__(self, idx):
        board = torch.from_numpy(self.data[idx][1].board).to(dtype=torch.float32).unsqueeze(0)
        death = torch.tensor(self.data[idx][0].death) - 1
        death = torch.clip(death, 0, 1)
        death = F.one_hot(death, 2).to(dtype=torch.float32)
        sample = (
            board,
            death,
        )
        return sample


def train():
    encoder = StateEncoder()
    encoder.initialise()

    predictor = DeathPredictor()
    predictor.initialise()

    batch_size = 32
    epochs = 10

    dataset = Dataset(DataProvider("data.bin"))

    dataloader = torch.utils.data.DataLoader(
        dataset, 
        batch_size=batch_size,
        shuffle=True, 
        num_workers=1
    )

    optimizer = torch.optim.Adam(
        [*encoder.weights, *predictor.weights],
        lr=0.0001
    )

    loss = torch.tensor([0.0], requires_grad=True)

    for epoch_i in range(epochs):
        epoch_loss = 0

        for batch_idx, data in enumerate(dataloader):
            inputs, targets = data

            optimizer.zero_grad()

            Vs = encoder(inputs)
            outputs = predictor(Vs)

            batch_loss = F.cross_entropy(outputs, targets)

            epoch_loss += batch_loss
            loss = torch.add(loss, batch_loss)

            loss.backward()

            optimizer.step()

        print("epoch loss: ", float(epoch_loss))

if __name__ == "__main__":
    train()