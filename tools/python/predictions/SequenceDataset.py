import torch
import torch.nn as nn
import torch.nn.functional as F
import json
from typing import List, Dict, Any


class SequenceDataset(torch.utils.data.Dataset):
    def __init__(self, data: List[Dict[str, Any]],
                 max_num_pairs=20,   # maximum K-V pairs per S_i to keep
                 embedding_dim=16,  # example dimension for each side (key or value)
                 pad_value=0):
        """
        data: list of dicts, where each dict has the key-value pairs for S_i.
        max_num_pairs: we will truncate or pad the number of pairs for each S_i to this size.
        embedding_dim: dimension for each encoder's latent vector
        pad_value: value to use for padding in the row dimension
        """
        super().__init__()
        self.data = data
        self.embedding_dim = embedding_dim
        self.max_num_pairs = max_num_pairs
        self.pad_value = pad_value

        # Placeholder for a "FastText" embedding of keys.
        # In reality, you would load pretrained embeddings here.
        self.key_embedding = nn.Embedding(10000, embedding_dim)

        # We create input-output pairs:
        #   X_0 = [S_0], Y_0 = S_1
        #   X_1 = [S_0, S_1], Y_1 = S_2
        #   ...
        #   X_i = [S_0, S_1, ..., S_i], Y_i = S_{i+1}
        # We'll store them as (list_of_S, next_S) in self.samples
        self.samples = []
        for i in range(len(data) - 1):
            # input states from S_0..S_i
            input_states = data[:i+1]
            # output state is S_{i+1}
            output_state = data[i+1]
            self.samples.append((input_states, output_state))

    def __len__(self):
        return len(self.samples)

    def __getitem__(self, idx):
        """
        Returns:
          (X_matrices, Y_matrix)
          Where X_matrices is a list of [S_0_matrix, S_1_matrix, ..., S_i_matrix]
          And Y_matrix is S_{i+1}_matrix
        """
        input_states, output_state = self.samples[idx]

        # Convert each S_i to a matrix
        X_matrices = [self.s_to_matrix(s) for s in input_states]
        # Convert S_{i+1} to a matrix
        Y_matrix = self.s_to_matrix(output_state)

        return X_matrices, Y_matrix

    def s_to_matrix(self, s: Dict[str, Any]) -> torch.Tensor:
        """
        Convert one JSON object (dict of key-value pairs) into a matrix of shape
        (max_num_pairs, 2 * embedding_dim), where we embed the key and the value
        then concatenate.

        Steps:
        - For each key, do FastText-like embedding (self.key_embedding).
        - For each value,
           if key ends with 'path' or 'exe': E_{v}^{path} (later, we can define the net).
           if value is string: E_{v}^{rest} (1D conv).
           else (numeric), expand to embedding_dim directly.
        - Mark the first element of the value embedding to indicate which encoder used
          (0 for direct numeric, 1 for rest, 2 for path).
        """
        # We'll gather up to self.max_num_pairs rows
        kv_pairs = list(s.items())[:self.max_num_pairs]
        matrix_rows = []

        for (k, v) in kv_pairs:
            key_vec = self.embed_key(k)  # shape (embedding_dim,)
            value_vec = self.embed_value(k, v)  # shape (embedding_dim,)
            row = torch.cat([key_vec, value_vec], dim=0)  # shape (2*embedding_dim,)
            matrix_rows.append(row)

        # Pad if needed
        if len(matrix_rows) < self.max_num_pairs:
            pad_length = self.max_num_pairs - len(matrix_rows)
            matrix_rows += [torch.zeros(2*self.embedding_dim) for _ in range(pad_length)]

        # Turn into tensor
        matrix = torch.stack(matrix_rows, dim=0)  # shape (max_num_pairs, 2*embedding_dim)
        return matrix

    def embed_key(self, key: str) -> torch.Tensor:
        """
        Example: we do a naive hash of the key to get an index, then do embedding lookup.
        In a real scenario, you'd probably tokenize the key string and use a pretrained embedding.
        """
        idx = abs(hash(key)) % 10000
        # shape (embedding_dim,)
        return self.key_embedding(torch.LongTensor([idx]))[0]  # single vector

    def embed_value(self, key: str, value: Any) -> torch.Tensor:
        """
        If key ends with exe or path => pass value to E_{v}^{path} (placeholder)
        If value is string => pass to E_{v}^{rest} (placeholder)
        Otherwise => numeric => expand to embedding_dim

        We'll also mark the first element of embedding to indicate encoder type:
          2 for path, 1 for rest, 0 for direct numeric
        """
        # Turn "value" into a string (some keys can be numeric, etc.)
        if isinstance(value, str):
            # Check if key ends with 'exe' or 'path'
            if key.endswith("exe") or key.endswith("path"):
                vec = self.e_v_path(value)
                # Mark first element = 2
                vec[0] = 2.
                return vec
            else:
                # general string
                vec = self.e_v_rest(value)
                # Mark first element = 1
                vec[0] = 1.
                return vec
        else:
            # assume numeric
            vec = self.e_v_numeric(value)
            # Mark first element = 0
            vec[0] = 0.
            return vec

    def e_v_path(self, text: str) -> torch.Tensor:
        # Placeholder for a small Transformer-based encoding
        # We'll just output a random vector for demonstration
        return torch.randn(self.embedding_dim)

    def e_v_rest(self, text: str) -> torch.Tensor:
        # Placeholder for a small 1D conv-based approach
        # We'll just output a random vector for demonstration
        return torch.randn(self.embedding_dim)

    def e_v_numeric(self, val: Any) -> torch.Tensor:
        # We handle numeric by just storing it in position 1, rest zeros
        vec = torch.zeros(self.embedding_dim)
        vec[1] = float(val) if isinstance(val, (int, float)) else 0.
        return vec
