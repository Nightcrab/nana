import os

os.environ["KERAS_BACKEND"] = "torch"

import keras
import numpy as np

# import sequential and dense
from keras.api.models import Sequential


def create_model():
    model = keras.Sequential()

    model.add(
        keras.layers.Conv2D(
            32,
            kernel_size=(3, 3),
            activation="relu",
            data_format="channels_last",
            input_shape=(10, 20, 1),
        )
    )

    model.add(keras.layers.Flatten())

    model.add(keras.layers.Dense(64, activation="relu"))
    model.add(keras.layers.Dense(2, activation="relu"))
    model.add(keras.layers.Softmax())

    return model


model = create_model()
print("Model created successfully!")
model.compile()
print("Model compiled successfully!")

# use the model and pass in a dummy input
x = model.call(keras.ops.ones((1, 20, 10, 1)))
print(x)
