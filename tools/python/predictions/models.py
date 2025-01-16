
class FastTextKeyEmbedder(nn.Module):
    def __init__(self, ft_dim=128):
        super().__init__()
        # In real usage, you'd load or store the FastText model here.
        # For demonstration, let's just do a dummy embedding with a learned table:
        # Key strings would be hashed or something similar.
        self.ft_dim = ft_dim
        self.dummy_key_embedding = nn.Embedding(1000, ft_dim)  # placeholder

    def forward(self, key_str: str) -> torch.Tensor:
        # This is a naive placeholder approach:
        # Convert key_str into an integer index (e.g., via hash)
        idx = abs(hash(key_str)) % 1000
        return self.dummy_key_embedding(torch.LongTensor([idx]))  # shape [1, ft_dim]

class ValuePathEncoder(nn.Module):
    def __init__(self, latent_dim=128, nhead=4, num_layers=2, vocab_size=1000, embed_dim=64):
        super().__init__()
        self.latent_dim = latent_dim
        # Let's do a naive embedding for characters
        self.embedding = nn.Embedding(vocab_size, embed_dim)
        encoder_layer = nn.TransformerEncoderLayer(d_model=embed_dim, nhead=nhead)
        self.transformer_encoder = nn.TransformerEncoder(encoder_layer, num_layers=num_layers)

        # We want to produce (latent_dim - 1) from final to then cat the marker
        self.fc = nn.Linear(embed_dim, latent_dim - 1)

    def forward(self, value_str: str) -> torch.Tensor:
        # tokenize
        token_ids = [abs(hash(ch)) % 1000 for ch in value_str]
        x = torch.LongTensor(token_ids).unsqueeze(1)  # shape [seq_len, 1]

        embedded = self.embedding(x)  # shape [seq_len, 1, embed_dim]

        # The nn.Transformer in PyTorch expects shape [seq_len, batch_size, d_model]
        # So we are good: [seq_len, batch_size, embed_dim]
        transformer_out = self.transformer_encoder(embedded)  # same shape

        # We might take the mean or the last token for the representation
        out_seq = transformer_out.mean(dim=0)  # shape [1, embed_dim]

        out_fc = self.fc(out_seq)  # shape [1, latent_dim-1]

        # Marker at index 0 for path. Let's say marker=1
        marker = torch.ones(1, 1)
        out = torch.cat([marker, out_fc], dim=1)  # shape [1, latent_dim]
        return out

class ValueRestEncoder(nn.Module):
    def __init__(self, latent_dim=128, vocab_size=1000, embed_dim=64):
        super().__init__()
        self.latent_dim = latent_dim
        self.embedding = nn.Embedding(vocab_size, embed_dim)
        # A small 1D conv to encode sequences:
        self.conv = nn.Conv1d(in_channels=embed_dim, out_channels=latent_dim - 1, kernel_size=3, padding=1)
        # We'll produce an output [latent_dim].
        # The +1 dimension is for the "which encoder" marker.

    def forward(self, value_str: str) -> torch.Tensor:
        """
        value_str is an arbitrary string that is not a path/exe
        For demonstration, we will just do an ad-hoc approach:
        1) Convert each character into a token index
        2) embed them
        3) pass through conv
        4) average pool or something
        """
        # Turn each character into index
        token_ids = [abs(hash(ch)) % 1000 for ch in value_str]
        x = torch.LongTensor(token_ids).unsqueeze(0)  # shape [1, seq_len]
        embedded = self.embedding(x)  # shape [1, seq_len, embed_dim]

        # For 1D conv, we want shape [batch, embed_dim, seq_len]
        embedded = embedded.permute(0, 2, 1)
        conv_out = self.conv(embedded)  # shape [1, latent_dim - 1, seq_len]

        # Let's do a simple mean over seq_len dimension
        pooled = conv_out.mean(dim=2)   # shape [1, latent_dim - 1]

        # Insert the marker in the 0th dimension (just cat a "1" or "2" etc.)
        marker = torch.zeros(1, 1)  # marker=0 means rest? or 1?
        # Let's say for rest we use marker=0
        out = torch.cat([marker, pooled], dim=1)  # shape [1, latent_dim]
        return out  # shape [1, latent_dim]

def numeric_value_to_vector(value: float, latent_dim=128, marker=2.0):
    """
    If the value is numeric, we place 'marker' in the first dimension
    and put value in second dimension, zeros for the rest.
    """
    vec = torch.zeros(latent_dim)
    vec[0] = marker
    if latent_dim > 1:
        vec[1] = value
    return vec.unsqueeze(0)  # shape [1, latent_dim]

class KeyValueEmbedder(nn.Module):
    def __init__(self,
                 key_embedder: FastTextKeyEmbedder,
                 rest_encoder: ValueRestEncoder,
                 path_encoder: ValuePathEncoder,
                 latent_dim=128,
                 key_dim=128):
        super().__init__()
        self.key_embedder = key_embedder
        self.rest_encoder = rest_encoder
        self.path_encoder = path_encoder
        self.latent_dim = latent_dim
        self.key_dim = key_dim

    def forward(self, key_str: str, value: Any) -> torch.Tensor:
        """
        Returns a single vector of shape [1, key_dim + latent_dim].
        """
        # 1) Key embedding (fasttext) -> shape [1, key_dim]
        k_emb = self.key_embedder(key_str)  # [1, ft_dim] (assuming ft_dim=key_dim)

        # 2) Decide how to embed the value
        if isinstance(value, str):
            # Check if key ends with 'exe' or 'path'
            if key_str.endswith("exe") or key_str.endswith("path"):
                v_emb = self.path_encoder(value)  # [1, latent_dim]
            else:
                v_emb = self.rest_encoder(value)  # [1, latent_dim]
        elif isinstance(value, int) or isinstance(value, float):
            # numeric
            # direct expansion
            v_emb = numeric_value_to_vector(float(value), latent_dim=self.latent_dim, marker=2.0)
        else:
            # fallback
            v_emb = numeric_value_to_vector(0.0, latent_dim=self.latent_dim, marker=3.0)

        # 3) Concat
        kv_emb = torch.cat([k_emb, v_emb], dim=1)  # [1, key_dim + latent_dim]
        return kv_emb

def build_matrix_from_dict(
    data_dict: Dict[str, Any],
    kv_embedder: KeyValueEmbedder
) -> torch.Tensor:
    """
    data_dict: one S_i
    returns a matrix: shape [num_key_value_pairs, key_dim + latent_dim]
    """
    rows = []
    for k, v in data_dict.items():
        kv_row = kv_embedder(k, v)  # [1, key_dim + latent_dim]
        rows.append(kv_row)
    return torch.cat(rows, dim=0)  # shape [num_pairs, key_dim + latent_dim]

class AutoEncoderEncoder(nn.Module):
    def __init__(self, input_dim, hidden_dim=256, nhead=4, num_layers=2):
        super().__init__()
        encoder_layer = nn.TransformerEncoderLayer(d_model=input_dim, nhead=nhead)
        self.transformer_encoder = nn.TransformerEncoder(encoder_layer, num_layers=num_layers)
        self.linear_out = nn.Linear(input_dim, hidden_dim)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """
        x: [seq_len, batch_size, input_dim] -> output: [batch_size, hidden_dim]
        We'll do a mean over seq_len after the encoder.
        """
        # x is [seq_len, batch_size, input_dim]
        encoded = self.transformer_encoder(x)  # same shape
        # mean over seq_len
        out = encoded.mean(dim=0)  # shape [batch_size, input_dim]
        out = self.linear_out(out) # shape [batch_size, hidden_dim]
        return out


class AutoEncoderDecoder(nn.Module):
    def __init__(self, hidden_dim=256, output_dim=256):
        super().__init__()
        # We want to reconstruct the entire [seq_len, d], but we only have a single vector hidden_dim
        # We'll just produce a fixed length output for demonstration or
        # we could do another Transformer with cross-attn.
        self.linear = nn.Linear(hidden_dim, output_dim)

    def forward(self, latent: torch.Tensor, seq_len: int) -> torch.Tensor:
        """
        latent: [batch_size, hidden_dim]
        We want to reconstruct a sequence of shape [seq_len, batch_size, output_dim].
        We'll do a naive approach: replicate 'latent' seq_len times, pass through linear,
        to produce each row's embedding. This is just a toy demo.
        """
        batch_size = latent.shape[0]
        repeated = latent.unsqueeze(1).repeat(1, seq_len, 1)  # shape [b, seq_len, hidden_dim]
        out = self.linear(repeated)  # [b, seq_len, output_dim]
        # reshape to [seq_len, b, output_dim]
        out = out.permute(1, 0, 2)
        return out


class LatentAggregator(nn.Module):
    def __init__(self, hidden_dim=256):
        super().__init__()
        self.rnn = nn.GRU(hidden_dim, hidden_dim, batch_first=True)

    def forward(self, latents: torch.Tensor) -> torch.Tensor:
        """
        latents: [batch_size, seq_len, hidden_dim] (or something similar)
        We'll return the last hidden state as l_i'.
        """
        output, h_n = self.rnn(latents)
        # h_n: [1, batch_size, hidden_dim]
        return h_n.squeeze(0)  # shape [batch_size, hidden_dim]

class NextStatePredictor(nn.Module):
    def __init__(self, hidden_dim=256, output_dim=256):
        super().__init__()
        self.linear = nn.Linear(hidden_dim, output_dim)

    def forward(self, latent_prime: torch.Tensor, seq_len: int) -> torch.Tensor:
        # Just like the auto-decoder, produce a [seq_len, batch_size, output_dim].
        batch_size = latent_prime.shape[0]
        repeated = latent_prime.unsqueeze(1).repeat(1, seq_len, 1)
        out = self.linear(repeated)
        out = out.permute(1, 0, 2)
        return out



class FullModel(nn.Module):
    def __init__(self, key_embed_dim=128, val_latent_dim=128, auto_hidden_dim=256, aggregator_hidden_dim=256):
        super().__init__()
        # Key/Value embedder
        self.key_embedder = FastTextKeyEmbedder(ft_dim=key_embed_dim)
        self.rest_encoder = ValueRestEncoder(latent_dim=val_latent_dim)
        self.path_encoder = ValuePathEncoder(latent_dim=val_latent_dim)
        self.kv_embedder = KeyValueEmbedder(
            self.key_embedder,
            self.rest_encoder,
            self.path_encoder,
            latent_dim=val_latent_dim,
            key_dim=key_embed_dim
        )

        # Autoencoder
        self.E_auto = AutoEncoderEncoder(input_dim=key_embed_dim + val_latent_dim,
                                         hidden_dim=auto_hidden_dim)
        self.D_auto = AutoEncoderDecoder(hidden_dim=auto_hidden_dim,
                                         output_dim=key_embed_dim + val_latent_dim)

        # Aggregator for latents -> produce l_i'
        self.latent_aggregator = LatentAggregator(hidden_dim=auto_hidden_dim)

        # Next state predictor
        self.D_pred = NextStatePredictor(hidden_dim=auto_hidden_dim,
                                         output_dim=key_embed_dim + val_latent_dim)

    def encode_state(self, S: Dict[str, Any]) -> torch.Tensor:
        """
        Build the matrix for a single state S, pass it through E_auto to get l_S
        """
        mat = build_matrix_from_dict(S, self.kv_embedder)  # [n, (key_dim+val_latent_dim)]
        # For transformer input, we want [seq_len, batch_size, d_model], let's do batch_size=1
        mat = mat.unsqueeze(1)  # [n, 1, d_model]
        l_S = self.E_auto(mat)  # [1, auto_hidden_dim]
        return l_S

    def decode_state(self, l_S: torch.Tensor, seq_len: int) -> torch.Tensor:
        """
        Reconstruct the matrix from l_S
        """
        recon = self.D_auto(l_S, seq_len)  # [seq_len, 1, key_dim+val_latent_dim]
        return recon

    def predict_next_state(self, latents: torch.Tensor, seq_len: int) -> torch.Tensor:
        """
        latents: [batch_size=1, i, auto_hidden_dim] for states {S0..S_{i-1}}
        Produce l_i' via aggregator, then decode into S_{i}
        """
        l_prime = self.latent_aggregator(latents) # shape [1, auto_hidden_dim]
        pred = self.D_pred(l_prime, seq_len)      # shape [seq_len, 1, key_dim+val_latent_dim]
        return pred

    def forward(self, input_sequence: List[Dict[str, Any]], target: Dict[str, Any]):
        """
        1) encode each S in input_sequence to get [l_0, l_1, ...]
        2) aggregator -> l_i'
        3) D_pred -> predicted matrix
        4) E_auto, D_auto for each state if we want to do autoencoder reconstruction
        5) compute losses
        """
        # 1) Encode
        latents = []
        for S in input_sequence:
            l_S = self.encode_state(S) # shape [1, auto_hidden_dim]
            latents.append(l_S)
        latents = torch.cat(latents, dim=0).unsqueeze(0)  # shape [1, i, auto_hidden_dim]

        # 2) Predict next state
        # We need a guess at seq_len for the next state.
        # For simplicity, let's do the same # of key-value pairs as target
        seq_len = len(target.keys())
        pred_matrix = self.predict_next_state(latents, seq_len)  # shape [seq_len, 1, d_model]

        # 3) Optionally also reconstruct each input state
        # to compute autoencoder reconstruction loss:
        ae_loss = 0.0
        for S in input_sequence:
            # encode
            l_S = self.encode_state(S)              # [1, auto_hidden_dim]
            mat = build_matrix_from_dict(S, self.kv_embedder)  # [n, d_model]
            n = mat.size(0)
            # decode
            recon_mat = self.decode_state(l_S, n)   # [n, 1, d_model]
            # compute L2 or something
            ae_loss += ((mat.unsqueeze(1) - recon_mat)**2).mean()

        # 4) Also measure prediction loss for the target
        target_mat = build_matrix_from_dict(target, self.kv_embedder) # [seq_len, d_model]
        pred_loss = ((target_mat.unsqueeze(1) - pred_matrix)**2).mean()

        loss = ae_loss + pred_loss
        return loss, ae_loss, pred_loss


# Example main
if __name__ == "__main__":
    json_data = [
        # your data...
    ]
    dataset = SequenceDataset(json_data, max_num_pairs=20, embedding_dim=16)
    loader = torch.utils.data.DataLoader(dataset, batch_size=2, shuffle=True)

    input_dim = 2*16  # Because we embed key and value each with dim=16
    latent_dim = 32
    max_num_pairs = 20

    model = FullModel(input_dim=input_dim, latent_dim=latent_dim, max_num_pairs=max_num_pairs)
    optimizer = torch.optim.Adam(model.parameters(), lr=1e-3)

    for epoch in range(10):
        loss_val = train_one_epoch(model, loader, optimizer)
        print(f"Epoch {epoch} Loss: {loss_val:.4f}")
