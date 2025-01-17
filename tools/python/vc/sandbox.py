import torch
import torch.nn.functional as F
from varchar import *

batch_strs = ["Hello", "World", "Test", "Example"]

e = StrEmbedder()

batch_embeddings = [e(b) for b in batch_strs]

max_length = max(tensor.shape[0] for tensor in batch_embeddings)
padding_value = 4
padded_batch = torch.stack([F.pad(tensor, (0, 0, 0, max_length - tensor.shape[0]), value=padding_value) for tensor in batch_embeddings])

batch = padded_batch


e = VarCharEncoder(d_q=32, d_k=32, embedding_dim=128, num_heads=3)
z = e(batch)

print(z)