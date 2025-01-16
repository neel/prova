import torch
import torch.nn as nn
import torch.nn.functional as F
import math 

class FixedPositionalEncoding(nn.Module):
    def __init__(self, D, max_len=5000):
        super(FixedPositionalEncoding, self).__init__()
        self.encoding = torch.zeros(max_len, 1, D)
        position = torch.arange(0, max_len, dtype=torch.float32).unsqueeze(1)
        div_term = torch.exp(torch.arange(0, D, 2).float() * (-math.log(10000.0) / D))
        self.pe = torch.zeros(max_len, D)
        self.pe[:, 0::2] = torch.sin(position * div_term)
        self.pe[:, 1::2] = torch.cos(position * div_term)
        
    def forward(self, x):
        pos_encoding = self.pe[:x.size(0)].detach()
        x = x + pos_encoding
        return x

class VarFeatureAttentionArm(nn.Module):
    def __init__(self, D, d_q=32, d_k=32):
        super(VarFeatureAttentionArm, self).__init__()
        self.d_q = d_q
        self.d_k = d_k
        self.W_q = nn.Parameter(torch.zeros(D, d_q))
        self.W_k = nn.Parameter(torch.zeros(D, d_k))

        nn.init.xavier_uniform_(self.W_q)
        nn.init.xavier_uniform_(self.W_k)

    def forward(self, X):
        # X.shape [L, D]
        Q = torch.matmul(X, self.W_q)               # Q.shape [L, d_q]
        K = torch.matmul(X, self.W_k)               # K.shape [L, d_k]
        h = torch.matmul(Q.transpose(-2, -1), K)    # h.shape [d_q, d_k]
        scale = math.sqrt(self.d_k)
        h = F.softmax(h / scale, dim=-1)
        return h

class VarFeatureAttention(nn.Module):
    def __init__(self, D, d_q=32, d_k=32, eta=128):
        super(VarFeatureAttention, self).__init__()
        self.D = D
        self.d_q = d_q
        self.d_k = d_k
        self.eta = eta
        
        self.a1 = VarFeatureAttentionArm(D, d_q, d_k)
        self.Omega = nn.Parameter(torch.zeros(d_k, eta))
        
        self.a2 = VarFeatureAttentionArm(D, d_q, d_k)
        self.zeta = nn.Parameter(torch.zeros(d_k, 1))

        nn.init.xavier_uniform_(self.Omega)
        nn.init.xavier_uniform_(self.zeta)
        
    def forward(self, X):
        h1 = self.a1(X)
        H = torch.matmul(h1, self.Omega)
        
        h2 = self.a2(X)
        xi = torch.matmul(h2, self.zeta)
        
        z = torch.matmul(H.transpose(-2, -1), xi)
        return z

class VarFeatureEncoder(nn.Module):
    def __init__(self, D=8, eta=128, d_q=32, d_k=32, num_heads=1):
        super(VarFeatureEncoder, self).__init__()
        self.D = D
        self.eta = eta
        self.d_q = d_q
        self.d_k = d_k
        self.num_heads = num_heads
        self.positional_encoding = FixedPositionalEncoding(D)
        self.heads = nn.ModuleList([VarFeatureAttention(D, d_q, d_k, eta) for _ in range(num_heads)])
        
    def forward(self, X):
        X = self.positional_encoding(X)
        z = torch.zeros(self.eta, 1, device=X.device)
        for head in self.heads:
            h = head(X)
            z += h
        return z / self.num_heads
    
class StrEmbedder(nn.Module):
    def __init__(self, D=6):
        super(StrEmbedder, self).__init__()
        self.D = D
        self.embedding = nn.Embedding(num_embeddings=128, embedding_dim=D)  
        self.char2idx = {}
        self.idx2char = {}
        codes = self.hardcodings()

        weight_data = torch.zeros(128, D)
        idx = 0
        for ch, code in codes.items():
            if idx < 128:
                weight_data[idx] = torch.tensor(code, dtype=torch.float)
                self.char2idx[ch] = idx
                self.idx2char[idx] = ch
                idx += 1
        self.embedding.weight = nn.Parameter(weight_data, requires_grad=False)

    def hardcodings(self):
        char_to_encoding = {
            ' ' : [0, 0, 0, 0, 0, 0],
            '"' : [1, 1, 0, 0, 0, 0],
            '(' : [1, 0, 1, 0, 0, 0],
            '[' : [1, 0, 0, 1, 0, 0],
            '{' : [1, 0, 0, 0, 1, 0],
            '<' : [1, 0, 0, 0, 0, 1],
            '\'': [1, 2, 0, 0, 0, 0],
            ')' : [1, 0, 2, 0, 0, 0],
            ']' : [1, 0, 0, 2, 0, 0],
            '}' : [1, 0, 0, 0, 2, 0],
            '>' : [1, 0, 0, 0, 0, 2],
            0   : [1, 1, 1, 1, 1, 1],
            1   : [2, 2, 2, 2, 2, 2]
        }
        punctuation_symbols = "!#$%&*+,-./:;=?@\\^_`|~"
        base_binary = 0  # Start counting from 0, this is arbitrary and ensures uniqueness
        for i, sym in enumerate(punctuation_symbols):
            binary = format(base_binary + i, '05b')  # Convert number to binary with 5 bits
            encoding = [2] + [int(b) for b in binary]  # Prepend '2' for x_5
            char_to_encoding[sym] = encoding
        
        for i in range(26):
            char = chr(ord('a') + i)
            # Create binary representation for character
            binary = format(i+1, '05b')  # Convert number to binary with 5 bits
            encoding = [0] + [int(b) for b in binary]  # Append the 0 at the start for x_5
            char_to_encoding[char] = encoding
        
        for i in range(26):
            char = chr(ord('A') + i)
            lowercase_encoding = char_to_encoding[chr(ord('a') + i)]
            encoding = [2*value for value in lowercase_encoding]
            char_to_encoding[char] = encoding

        char_to_encoding['0'] = [0, 1, 1, 0, 1, 1]
        char_to_encoding['1'] = [0, 1, 1, 1, 0, 0]
        char_to_encoding['2'] = [0, 1, 1, 1, 0, 1]
        char_to_encoding['3'] = [0, 1, 1, 1, 1, 0]
        char_to_encoding['4'] = [0, 1, 1, 1, 1, 1]
        char_to_encoding['5'] = [0, 2, 2, 0, 2, 2]
        char_to_encoding['6'] = [0, 2, 2, 2, 0, 0]
        char_to_encoding['7'] = [0, 2, 2, 2, 0, 2]
        char_to_encoding['8'] = [0, 2, 2, 2, 2, 0]
        char_to_encoding['9'] = [0, 2, 2, 2, 2, 2]

        return char_to_encoding

    def forward(self, txt: str):
        x = self.encode_chars(txt)
        x = self.embedding(x)
        return x
    
    def encode_chars(self, txt: str):
        indices = []
        for ch in txt:
            if ch in self.char2idx:
                indices.append(self.char2idx[ch])
            else:
                # Fallback (could be 0, or anything you prefer)
                indices.append(0)
        
        indices = torch.tensor(indices, dtype=torch.long)
        return indices

    def decode_str(self, x):
        decoded = []
        for z in x:
            dist = torch.norm(self.embedding.weight - z, dim=1)
            idx  = torch.argmin(dist)
            decoded.append(idx.item())

        chars = [self.idx2char[i] for i in decoded]
        str = "".join(chars)
        return str

class VarCharFeatureEncoder(nn.Module):
    def __init__(self, embedder, eta=128, d_q=32, d_k=32, num_heads=1):
        super(VarCharFeatureEncoder, self).__init__()
        self.embedding = embedder  
        self.feature_encoder = VarFeatureEncoder(embedder.D, eta, d_q, d_k, num_heads)
        
    def forward(self, input, output = None):
        x = self.embedding(input)
        z = self.feature_encoder(x)
        if output is None:
            return z
        else:
            return z, self.embedding(output)

class FixedCharGumbelDecoder(nn.Module):
    """
    Transforms a latent space code of length \eta to a vector of size 128 each having integer values between [0, 127] 
    """
    def __init__(self, embedder, eta=128, num_heads=1):
        super(FixedCharGumbelDecoder, self).__init__()
        self.embedder = embedder
        self.positional_encoding = FixedPositionalEncoding(1)
        self.a = VarFeatureAttentionArm(1, 128, 128)

    def forward(self, z):
        z = self.positional_encoding(z) # x.shape = [128]
        z = self.a(z)                   # x.shape = [128, 128]
        z = F.relu(z)
        z = F.gumbel_softmax(z, 0.08, hard=False)
        z = torch.matmul(z, self.embedder.embedding.weight) # [128, D]
        return z
    
def train(input, output, embedder: StrEmbedder):
    enc = VarCharFeatureEncoder(embedder, 128, 16, 32)
    dec = FixedCharGumbelDecoder(embedder)

    z, output_encoded = enc(input, output)
    output = dec(z)

    return nn.MSELoss(output_encoded, output)


