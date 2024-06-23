
# This file is used to train the neural network to predict the death of the player.
# it is also an old file, and we will be switching to c++ pytorch for the training.

import os

os.environ["KERAS_BACKEND"] = "torch"

import keras
import numpy as np


def reformat_bytes_to_ints(data):
    """
    Reformats a byte string (`data`) into a list of 4-byte integers.

    Args:
        data: A bytes object containing 10 bytes.

    Returns:
        A list of 10 integers, each representing 4 bytes from the original data.

    Raises:
        ValueError: If the length of the data is not a multiple of 4.
    """

    if len(data) % 4 != 0:
        raise ValueError("Input data length must be a multiple of 4 bytes.")

    # Use struct module to unpack bytes into integers
    int_array = np.frombuffer(data, dtype=np.uint32).reshape(-1)
    uint32_array = int_array.view(dtype=np.uint8)

    # Unpack each integer into 4 bytes (8 bits)
    unpacked_bytes = np.unpackbits(uint32_array, axis=0)

    # Convert unpacked bytes to booleans (True for 1, False for 0)
    return unpacked_bytes.reshape(10, 32).astype(np.float32)


"""
struct data{
    // the board is 10 ints
    Board b;

    // current piece type
    u8 p_type;

    // move
    u8 m_type;
    u8 m_rot;
    u8 m_x;
    u8 m_y;

    // extra data
    u8 meter;
    u8 attack;
    u8 damage_received;
    u8 spun;
    u8 queue[5];
    u8 hold;
};
52 bytes
"""


class State:
    def __init__(
        self,
        board,
        piece_type,
        move_type,
        move_rotation,
        move_x,
        move_y,
        meter,
        sent,
        received,
        spin,

        queue,
        hold_type,
    ):
        # read four byte in the board as one integer

        self.board = reformat_bytes_to_ints(board)
        self.piece_type = piece_type
        self.move_type = move_type
        self.move_rotation = move_rotation
        self.move_x = move_x
        self.move_y = move_y
        self.meter = meter
        self.sent = sent
        self.received = received
        self.spin = spin
        self.queue = queue
        self.hold_type = hold_type


class Death:
    def __init__(self, death):
        self.death = death


class DataProvider:
    def __init__(self, file_name):
        # open file as bytes
        self.file = open(file_name, "rb")

    def get_next_data(self) -> tuple[Death, State, State] | None:
        data = self.file.read(55 + 55 + 1 + 1 + 1)
        if len(data) == 0:
            return None
        return (
            Death(data[0]),
            State(
                data[1:41],
                data[41],
                data[42],
                data[43],
                data[44],
                data[45],
                data[46],
                data[47],
                data[48],
                data[49],
                data[50:55],
                data[55],
            ),
            State(
                data[53:93],
                data[93],
                data[94],
                data[95],
                data[96],
                data[97],
                data[98],
                data[99],
                data[100],
                data[101],
                data[102:107],
                data[107],
            ),
        )

    def get_games_data_set(self) -> list[tuple[Death, State, State]]:
        data_set = []

        while True:
            data = self.get_next_data()

            if data is None:
                return data_set

            data_set.append(data)


inputs = keras.Input(shape=(10, 32, 1))

conv2d = keras.layers.Conv2D(
    32, kernel_size=(3, 3), activation="relu", data_format="channels_last"
)(inputs)

conv2d2 = keras.layers.Conv2D(
    32, kernel_size=(3, 3), activation="relu", data_format="channels_last"
)(conv2d)

conv2d3 = keras.layers.Conv2D(
    32, kernel_size=(3, 3), activation="relu", data_format="channels_last"
)(conv2d2)

pool = keras.layers.AveragePooling2D(pool_size=(2, 2))(conv2d3)

flatten = keras.layers.Flatten()(pool)

# outputs the V vector :-)
shared_vector = keras.layers.Dense(64, activation="relu", name="V")(flatten)


# state predictor
# concat a number called "attack" to the shared_vector
attack = keras.Input(shape=(1,))
concatenated = keras.layers.Concatenate(axis=1)([attack, shared_vector])
raw_output_state = keras.layers.Dense(64, activation="relu")(concatenated)


# attack predictor
raw_output_attack = keras.layers.Dense(20, activation="relu")(shared_vector)
attack_normalized = keras.layers.Softmax()(raw_output_attack)
attack_model = keras.Model(
    inputs=inputs, outputs=attack_normalized, name="Attack Predictor"
)

# death predictor
raw_output_death = keras.layers.Dense(2, activation="relu", name="raw_output")(
    shared_vector
)
death_normalized = keras.layers.Softmax()(raw_output_death)
death_model = keras.Model(
    inputs=inputs, outputs=death_normalized, name="Death Predictor"
)

death_model.compile(
    optimizer=keras.optimizers.Adam(learning_rate=1e-4),
    loss="categorical_crossentropy",
    metrics=[keras.metrics.TopKCategoricalAccuracy(k=1)],
)

data_set = DataProvider("data.bin").get_games_data_set()


attack_states = np.array([data[1].meter for data in data_set])

# death_states = np.array([data[0].death for data in data_set])
# death_states = death_states - 1

# y_train = np.array([data[0].death for data in data_set])
# y_train = keras.utils.to_categorical(death_states.clip(0, 1), num_classes=2)

# # concat all of the dataset into a (N, 10, 20, 1) numpy array
# x_train = np.array([data[1].board for data in data_set])

# death_model.fit(x_train, y_train, epochs=100, batch_size=4096)
# print("size of the dataset: ", len(data_set))
# death_model.save("death_predictor.keras")
