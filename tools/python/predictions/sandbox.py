import numbers
import json
import torch
import torch.nn as nn
from transformers import BertModel, BertTokenizer
import fasttext.util
import mmh3
import math

import matplotlib.pyplot as plt
import numpy as np

from SyscallsDataset import SyscallsDataset
from features import *

# fasttext.util.download_model('en', if_exists='ignore')
# ft_model = fasttext.load_model('cc.en.300.bin')

# # Define tokenizer for paths
# tokenizer   = BertTokenizer.from_pretrained('bert-base-uncased')
# bert_model  = BertModel.from_pretrained('bert-base-uncased')

class PositionalEncoder(nn.Module):
    def __init__(self, d_model, dropout=0.1, max_len=256):
        super(PositionalEncoder, self).__init__()
        self.dropout = nn.Dropout(p=dropout)

        pe = torch.zeros(max_len, d_model)
        position = torch.arange(0, max_len, dtype=torch.float).unsqueeze(1)
        div_term = torch.exp(torch.arange(0, d_model, 2).float() * (-math.log(10000.0) / d_model))
        pe[:, 0::2] = torch.sin(position * div_term)
        pe[:, 1::2] = torch.cos(position * div_term)
        pe = pe.unsqueeze(0).transpose(0, 1)
        self.register_buffer('pe', pe)

    def forward(self, x):
        x = x + self.pe[:x.size(0), :]
        return self.dropout(x)

class PathEmbedder(nn.Module):
    def __init__(self, d_model=128, max_seq_length=256):
        super().__init__()
        self.char_embedding = nn.Embedding(128, d_model)
        self.pos_encoder  = PositionalEncoder(d_model=d_model, dropout=0.1)
        self.path_encoder = nn.TransformerEncoder(
            nn.TransformerEncoderLayer(d_model=d_model, nhead=2),
            num_layers=1
        )
        self.attention_weights = nn.Parameter(torch.rand(max_seq_length))

    def forward(self, x):
        x = self.char_embedding(x)
        print("char embedding shape", x.shape)
        x = self.pos_encoder(x)
        x = self.path_encoder(x)
        print("self.path_encoder(x) shape", x.shape)
        scores = torch.softmax(self.attention_weights[:x.size(1)], dim=0)  # Make sure to slice to the actual sequence length
        scores = scores.unsqueeze(0).unsqueeze(2).expand_as(x)  # Expand scores to match the embeddings' shape
        x = (x * scores).sum(dim=1)  # Sum weighted embeddings across the sequence
        return x
        
class KVEmbedder(nn.Module):
    def __init__(self, dataset, key_embedding_dim = 10, d_model=128):
        super().__init__()
        self.dataset      = dataset
        self.fixed_keys   = dataset.fixed_keys()
        self.d_model      = d_model
        self.key_embedding_dim = key_embedding_dim
        num_keys          = len(self.fixed_keys)

        self.key_embedder  = nn.Embedding(num_keys, key_embedding_dim)
        self.path_embedder = PathEmbedder(d_model)

    def all_indices(self):
        with torch.no_grad():
            indices = [self.dataset.fixed_key(key) for key in self.fixed_keys]
            return self.key_embedder(indices)

    def forward(self, x):
        key    = x["k"]
        vtype  = x["t"]
        value  = x["v"]

        key_tensor = torch.tensor([key], dtype=torch.long)
        key_embedding = self.key_embedder(key_tensor)
        print("key_embedding.shape", key_embedding.shape)

        marker = torch.zeros((1, 3))
        if vtype == "path":      # path value
            char_indices = torch.tensor([ord(char) for char in value], dtype=torch.long).unsqueeze(0)
            path_embedding = self.path_embedder(char_indices)
            print("path_embedding.shape", path_embedding.shape)
            value = path_embedding
            marker[0, 0] = 1
        elif vtype == "word":
            value_embedding = self.key_embedder(torch.tensor([value], dtype=torch.long)).squeeze(0)
            padding = torch.tensor([0] * (self.d_model - self.key_embedding_dim)).unsqueeze(0)
            print("value_embedding.shape", value_embedding.shape)
            print("padding.shape", padding.shape)
            value = torch.cat((value_embedding, padding), dim=1)
            marker[0, 1] = 1
        else:
            marker[0, 2] = 1
            value = [int(value)] + [0] * (self.d_model -1)
            value = torch.tensor(value).unsqueeze(0)
        
        print("marker.shape", marker.shape)
        print("value.shape", value.shape)
        return torch.cat((key_embedding, marker, value), dim=1)

# Example of how to use the KeyValueEmbedder
# embedder = KeyValueEmbedder()

# Example data
# data = [
#     {
#         "process.exe": "/usr/lib/postgresql/13/bin/postgres",
#         "artifact.path": "/var/run/postgresql/13-main.pg_stat_tmp/global.tmp",
#         "action.operation": "open",
#         "process.name": "postgres",
#         "process.pid": 42,
#         # More key-value pairs...
#     }
# ]

# Embedding each key-value pair in the dataset
# for action in data:
#     embeddings = []
#     for key, value in action.items():
#         embeddings += [embedder(key, value)]

# embedding_matrix = torch.stack(embeddings)

# print(embedding_matrix.shape)

dataset = SyscallsDataset("/home/sunanda/Projects/provenance/build/tools/ui/")
for key, value in dataset.fixed_keys().items():
    print("{:<40} {:<10}".format(key, value))

print("")

dataset.adjust_keys()
for key, value in dataset.fixed_keys().items():
    print("{:<40} {:<10}".format(key, value))

# print("total", len(dataset))
# print("ExU#1", len(dataset[0]))
# print("ExU#1/1", dataset[0][0])
# print("ExU#1/2", dataset[0][1])

print(dataset[0][0][12])

kv_emvedder = KVEmbedder(dataset)
embedding = [kv_emvedder(dataset[0][0][i]) for i in range(len(dataset[0][0]))]
embedding_tensor = torch.stack(embedding, dim=1)
print("embedding_tensor.shape", embedding_tensor.shape)

# embedder_path = PathEmbedder()
# path = "path/to/some/file"
# char_indices = torch.tensor([ord(char) for char in path], dtype=torch.long).unsqueeze(0)  # Convert characters to ASCII values
# print("char_indices.shape", char_indices.shape)
# path_embedding = embedder_path(char_indices)
# print("path_embedding.shape", path_embedding.shape)

str_embedder = StrEmbedder()
hardcodes = str_embedder.hardcodings()

# Initialize the color map array
cmap = np.zeros((len(hardcodes), 6, 3))

# Collect keys for labels and sort to maintain consistent order
keys = list(hardcodes.keys())
# keys.sort()

# Fill the color map based on the encoding values
for i, key in enumerate(keys):
    encoding = hardcodes[key]
    for j, val in enumerate(encoding):
        if val == 0:
            cmap[i, j] = [1, 1, 1]  # white
        elif val == 1:
            cmap[i, j] = [0, 0, 1]  # blue
        elif val == 2:
            cmap[i, j] = [1, 0, 0]  # red

# Plotting the matrix
fig, ax = plt.subplots(figsize=(10, 10))
ax.imshow(cmap, aspect='auto')
ax.set_xticks(range(6))
ax.set_yticks(range(len(hardcodes)))
ax.set_xticklabels(['x5', 'x4', 'x3', 'x2', 'x1', 'x0'])
ax.set_yticklabels([f'{key}' for key in keys])  # Label with actual character
ax.set_title('Visual Encoding Matrix')
plt.show()

for k, v in hardcodes.items():
    print(k, v)

full_str = "0123456789 abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!#$%&*+,-./:;=?@\\^_`|~\"'(){}<>[]"
embeddings = str_embedder(full_str)
print(embeddings)

# vc_embedder = VarCharFeatureEncoder(str_embedder, 128, 16, 32)
# print(vc_embedder)

# embeddings = vc_embedder("Hello World").squeeze(1)
# print(embeddings)


decoded = str_embedder.decode_str(embeddings)
print(decoded)
print(full_str)
print(decoded == full_str)
# decoder = FixedCharDecoder(str_embedder)