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

    def load_weights(self, filename):
        packed = torch.load(filename)
        self.weights = packed['weights']
        self.biases = packed['biases']

    def save_weights(self, filename):
        packed = {
            'weights' : self.weights,
            'biases' : self.biases,
        }
        torch.save(packed, filename)
    
    def initialise(self):
        for weight in self.weights:
            torch.nn.init.normal_(weight, mean=0.0, std=1.0)
        for bias in self.biases:
            torch.nn.init.zeros_(bias)


class AttackEmbedding(NN):
    """
    vector embedding for attacks
    integer (between 0 and 32) -> 2-vector
    """
    def __init__(self):
        self.weights = [
            parameter(32, 2)
        ]

        self.biases = []

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
    64 state vector, attack 2-vector -> 64 vector
    """
    def __init__(self):
        self.weights = [
            parameter(128, 64 + 2),
            parameter(64, 128)
        ]

        self.biases = [
            parameter(128),
            parameter(64)
        ]

    def __call__(self, v: torch.Tensor, atk: torch.Tensor):

        x = torch.concat((v, atk), dim=-1)
        x = F.linear(x, self.weights[0], bias=self.biases[0])
        x = F.relu(x)
        x = F.linear(x, self.weights[1], bias=self.biases[1])
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

class Decoder(NN):
    """
    Tries to reconstruct a board from a latent vector.
    Used for autoregressive training of the encoder.
    64 vector -> 10x20 board
    """
    def __init__(self):
        self.weights = [
            parameter(320, 64)
        ]

        self.biases = [
            parameter(320)
        ]

    def __call__(self, v: torch.Tensor):
        x = F.linear(v, self.weights[0], bias=self.biases[0])
        x = F.tanh(x)
        x = torch.reshape(x, (-1, 1, 10, 32))

        return x


class Dataset(torch.utils.data.Dataset):
    def __init__(self, provider: DataProvider):

        raw_data: list[tuple[Death, State, State]] = provider.get_games_data_set()
        self.board_data = []
        self.data = []

        for idx in range(len(raw_data) // 10):
            board = torch.from_numpy(raw_data[idx][1].board).to(dtype=torch.float32).unsqueeze(0)
            death = torch.tensor(raw_data[idx][0].death) - 1
            death = torch.clip(death, 0, 1)
            death = F.one_hot(death, 2).to(dtype=torch.float32)

            self.board_data.append((death, board))
        
        for idx in range(len(raw_data) // 10 - 1):
            death, board = self.board_data[idx]
            death_, board_ = self.board_data[idx + 1]

            received = raw_data[idx + 1][1].received
            received = torch.tensor(received, dtype=torch.int)

            tpl = ((death, board), received, (death_, board_))

            self.data.append(tpl)

    def __len__(self):
        return len(self.data) // 10

    def __getitem__(self, idx):
        sample = self.data[idx]
        return sample


def train(use_saved=True):

    encoder = StateEncoder()
    predictor_d = DeathPredictor()
    embedding = AttackEmbedding()
    predictor_s = StatePredictor()
    decoder = Decoder()

    if use_saved:
        encoder.load_weights("weights/encoder.pt")
        predictor_d.load_weights("weights/predictor_d.pt")
        predictor_s.load_weights("weights/predictor_s.pt")
        embedding.load_weights("weights/embedding.pt")
        decoder.load_weights("weights/decoder.pt")
    else:
        decoder.initialise()
        predictor_s.initialise()
        encoder.initialise()
        predictor_d.initialise()
        embedding.initialise()

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
        [
            *encoder.weights, 
            *encoder.biases, 
            *predictor_d.weights, 
            *predictor_d.biases,
            *embedding.weights,
            *predictor_s.weights,
            *predictor_s.biases,
            *decoder.weights,
            *decoder.biases,
        ],
        lr=0.0001
    )

    for epoch_i in range(epochs):
        epoch_loss = 0

        for batch_idx, data in enumerate(dataloader):
            pair1, attack, pair2 = data

            dead, board = pair1
            _, next_board = pair2

            optimizer.zero_grad()

            batch_loss = torch.tensor([0.0], requires_grad=True)

            # encode board for V
            V = encoder(board)

            # encode next board for V'
            V_ = encoder(next_board)

            # embed attack
            atk_vec = embedding(attack)

            # predict Dead(V)
            dead_predicted = predictor_d(V)

            # predict V'
            V_predicted = predictor_s(V, atk_vec)

            # decode V
            board_decoded = decoder(V)

            # loss of predictor_d
            batch_loss = torch.add(batch_loss, F.cross_entropy(dead_predicted, dead))

            # loss of predictor_s
            batch_loss = torch.add(batch_loss, F.mse_loss(V_, V_predicted))

            # loss of decoder
            batch_loss = torch.add(batch_loss, F.mse_loss(board_decoded, board))

            epoch_loss += batch_loss

            batch_loss.backward()

            optimizer.step()

        print("epoch loss: ", float(epoch_loss))

        encoder.save_weights("weights/encoder.pt")
        predictor_d.save_weights("weights/predictor_d.pt")
        predictor_s.save_weights("weights/predictor_s.pt")
        embedding.save_weights("weights/embedding.pt")
        decoder.save_weights("weights/decoder.pt")

if __name__ == "__main__":
    train()