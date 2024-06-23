import torch

from torch.nn import functional as F

class AttackEmbedding:
    def __init__(weights):
        self.weight = weights[0]

    def __call__(attack: int):
        return F.embedding(attack, self.weight)

class StateEncoder:
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

    def load_weights(self, weights):
        self.weights = weights
    
    def initialise(self):
        for weight in self.weights:
            torch.nn.init.kaiming_uniform_(weight, mode='fan_in', nonlinearity='relu')

    def __call__(self, board: torch.Tensor):
        # 32 channels, 3x3 kernels
        x = F.conv2d(board, self.weights[0], bias=None, stride=1, padding=0)
        x = F.relu(x)
        
        # 32 channels, 3x3 kernels
        x = F.conv2d(x, self.weights[1], bias=None, stride=1, padding=0)
        x = F.relu(x)

        # 32 channels, 3x3 kernels
        x = F.conv2d(x, self.weights[2], bias=None, stride=1, padding=0)
        x = F.relu(x)
        
        # average pooling
        x = F.avg_pool2d(x, (2,2))

        # flatten 
        v = torch.flatten(x)

        return v


class DeathPredictor:
    """
        64 state vector -> 2 categorical
    """
    def __init__():
        pass
    def __call__(vector: torch.Tensor):
        pass

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


encoder = StateEncoder()

encoder.initialise()

x = torch.zeros(1, 10, 20)

print(encoder(x))