import torch
import torch.nn as nn
import torch.nn.functional as F
import math 

class NormalizedProjector(nn.Module):
    def __init__(self):
        super(NormalizedProjector, self).__init__()
    def forward(self, x, w):
        # assert x.shape[-2] == w.shape[-1], "The dimensions of x and w are incompatible for matrix multiplication."

        scale = torch.sqrt(torch.tensor(w.shape[-2], dtype=torch.float32))

        x = torch.matmul(x, w)
        # x = x / scale
        x = F.softmax(x, dim=1)
        return x

class GenericAttention(nn.Module):
    def __init__(self):
        super(GenericAttention, self).__init__()
        self.projector = NormalizedProjector()

    def forward(self, Q, K):
        # assert Q.shape[1] == K.shape[1], "The dimensions of Q and K are incompatible in the second dimension."

        Kt = K.transpose(-2, -1)
        return self.projector(Q, Kt)
    
class GenericCoAttention(nn.Module):
    def __init__(self):
        super(GenericCoAttention, self).__init__()
        self.projector = NormalizedProjector()

    def forward(self, Q, K):
        assert Q.shape[0] == K.shape[0], "The dimensions of Q and K are incompatible in the first dimension."

        Qt = Q.transpose(-2, -1)
        return self.projector(Qt, K)
    
class Attention(nn.Module):
    def __init__(self, D, u):
        super(Attention, self).__init__()
        self.D = D 
        self.u = u 
        
        self._attention = GenericAttention()
        self._phi = nn.Parameter(torch.zeros(D, u))
        self._psi = nn.Parameter(torch.zeros(D, u))

        nn.init.xavier_uniform_(self._phi)
        nn.init.xavier_uniform_(self._psi)

    def forward(self, x):
        assert x.shape[1] == self.D, ("X must be of the shape [L, {D}] for some L, but it is of the shape [{L}, {D}]").format(L=x.shape[0], D=self.D) 

        x_phi = torch.matmul(x, self._phi)
        x_psi = torch.matmul(x, self._psi)
        return self._attention(x_phi, x_psi)
    
class CoAttention(nn.Module):
    def __init__(self, D, u, v):
        super(CoAttention, self).__init__()
        self.D = D 
        self.u = u 
        self.v = v 
        self._co_attention = GenericCoAttention()
        self._phi = nn.Parameter(torch.zeros(D, u))
        self._psi = nn.Parameter(torch.zeros(D, v))

        nn.init.xavier_uniform_(self._phi)
        nn.init.xavier_uniform_(self._psi)

    def forward(self, x):
        assert x.shape[-1] == self.D, ("X must be of the shape [L, {D}] for some L, but it is of the shape [{L}, {D}]").format(L=x.shape[0], D=self.D) 

        x_phi = torch.matmul(x, self._phi)
        x_psi = torch.matmul(x, self._psi)

        return self._co_attention(x_phi, x_psi)
    
class DualHeadCondenser(nn.Module):
    def __init__(self, eta, v):
        super(DualHeadCondenser, self).__init__()
        self.eta = eta 
        self.v = v

        self._omega = nn.Parameter(torch.zeros(v, eta))
        self._zeta  = nn.Parameter(torch.zeros(v, 1))

        nn.init.xavier_uniform_(self._omega)
        nn.init.xavier_uniform_(self._zeta)

    def forward(self, h1, h2):
        assert h1.shape[0] == h2.shape[0], "Both h1 and h2 must have same size in the first dimension"
        assert h1.shape[1] == self.v, "Incompatible h1, it must be of the shape [_, {}]".format(self.v)
        assert h2.shape[1] == self.v, "Incompatible h2, it must be of the shape [_, {}]".format(self.v)

        h1 = torch.matmul(h1, self._omega)
        h2 = torch.matmul(h2, self._zeta)
        h1T = h1.transpose(-2, -1)
        h  = torch.matmul(h1T, h2)
        return h 
    
class FeatureCrossover(nn.Module):
    def __init__(self, eta, zeta, u):
        super(FeatureCrossover, self).__init__()
        self.eta  = eta
        self.zeta = zeta 
        self.u    = u

        self._phi = nn.Parameter(torch.zeros(zeta, u))
        self._psi = nn.Parameter(torch.zeros(eta, u))

        nn.init.xavier_uniform_(self._phi)
        nn.init.xavier_uniform_(self._psi)

        self._attention = GenericAttention()

    def forward(self, m1, m2):
        # assert m1.shape == [self.eta, self.zeta], "Incompatible m1, it must be of the shape [{}, {}]".format(self.eta, self.zeta)
        # assert m2.shape == [self.eta, self.zeta], "Incompatible m2, it must be of the shape [{}, {}]".format(self.eta, self.zeta)

        hm1 = torch.matmul(m1, self._phi)
        hm2 = torch.matmul(m2.transpose(-2, -1), self._psi)
        return self._attention(hm1, hm2)
    
class CrossCrossoverCovarianceAttention(nn.Module):
    def __init__(self, eta, zeta, tau, u):
        super(CrossCrossoverCovarianceAttention, self).__init__()
        self.eta  = eta
        self.zeta = zeta 
        self.tau  = tau
        self.u    = u

        self._wq    = nn.Parameter(torch.zeros(tau, zeta))
        self._wk    = nn.Parameter(torch.zeros(tau, zeta))
        self._phi_q = nn.Parameter(torch.zeros(zeta, u))
        self._phi_k = nn.Parameter(torch.zeros(zeta, u))
        self._psi_q = nn.Parameter(torch.zeros(eta, u))
        self._psi_k = nn.Parameter(torch.zeros(eta, u))

        self._crossover_q  = FeatureCrossover(eta, zeta, u)
        self._crossover_k  = FeatureCrossover(eta, zeta, u)
        self._co_attention = GenericCoAttention()


        nn.init.xavier_uniform_(self._wq)
        nn.init.xavier_uniform_(self._wk)
        nn.init.xavier_uniform_(self._phi_q)
        nn.init.xavier_uniform_(self._phi_k)
        nn.init.xavier_uniform_(self._psi_q)
        nn.init.xavier_uniform_(self._psi_k)

    def forward(self, z1, z2):
        # assert z1.shape[1:] == [self.eta, self.tau], "Incompatible z1, it must be of the shape [{}, {}]".format(self.eta, self.tau)
        # assert z2.shape[1:] == [self.eta, self.tau], "Incompatible z2, it must be of the shape [{}, {}]".format(self.eta, self.tau)

        Qp = self._crossover_q(
            torch.matmul(z1, self._wq),
            torch.matmul(z2, self._wq)
        )
        Kp = self._crossover_k(
            torch.matmul(z1, self._wk),
            torch.matmul(z2, self._wk)
        )
        
        A = self._co_attention(Qp, Kp)
        Ar = F.relu(A)
        # Zg = F.gumbel_softmax(Ar, 0.2, dim=1, hard=False)
        Zg = F.softmax(Ar, dim=1)
        return Zg
    
class Enricher(nn.Module):
    def __init__(self, tau):
        super(Enricher, self).__init__()
        self.tau = tau 

        self._beta = nn.Parameter(torch.zeros(1, tau))
        self._s    = nn.Parameter(torch.zeros(1, tau))
        self._mu   = nn.Parameter(torch.zeros(2*tau, tau))

        nn.init.xavier_uniform_(self._beta)
        nn.init.xavier_uniform_(self._s)
        nn.init.xavier_uniform_(self._mu)

    def forward(self, z):
        z_beta = torch.matmul(z, self._beta)
        z_s    = torch.matmul(z, self._s)
        z_s    = F.softmax(z_s, dim=1)
        cat    = torch.cat([z_beta, F.relu(z_s)], dim=-1)

        return torch.matmul(cat, self._mu)


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
        seq_length = x.size(1)
        pos_encoding = self.pe[:seq_length]
        pos_encoding = pos_encoding.unsqueeze(0)
        x = x + pos_encoding
        return x
    
class VarEncoder(nn.Module):
    def __init__(self, D, d_q, d_k, embedding_dim, num_heads):
        super(VarEncoder, self).__init__()
        self.num_heads = num_heads
        self.D      = D
        self.eta    = embedding_dim 
        self.d_q    = d_q
        self.d_k    = d_k
        
        self._pos_encoder = FixedPositionalEncoding(D)
        self._attentions  = nn.ModuleList([CoAttention(D, d_q, d_k) for _ in range(num_heads)])
        self._condensers  = nn.ModuleList([DualHeadCondenser(embedding_dim, d_k) for _ in range(num_heads-1)])

    def forward(self, x):
        assert x.shape[-1] == self.D
        
        x = self._pos_encoder(x)
        a = [att(x) for att in self._attentions]
        h = [self._condensers[i](a[i], a[i+1]) for i in range(self.num_heads - 1)]
        H = torch.mean(torch.stack(h), dim=0)
        return H

class VarDecoder(nn.Module):
    def __init__(self, embedding_dim, expanded_dim, dictionary_length, u):
        super(VarDecoder, self).__init__()
        self.eta = embedding_dim
        self.tau = expanded_dim 

        self._pos_encoder   = FixedPositionalEncoding(1)
        self._enricher_main = Enricher(expanded_dim)
        self._enricher_sub  = Enricher(expanded_dim)
        self._cross_co_attention = CrossCrossoverCovarianceAttention(eta=embedding_dim, zeta=dictionary_length, tau=expanded_dim, u=u)

    def forward(self, z1, z2):
        assert z1.shape == z2.shape
        
        z1 = self._pos_encoder(z1)
        z2 = self._pos_encoder(z2)
        z1 = self._enricher_main(z1)
        z2 = self._enricher_sub(z2)
        y  = self._cross_co_attention(z1, z2)
        return y
    

class StrEmbedder(nn.Module):
    """
    Hard-embeds all ASCII characters into a vector of 6 elements. Not Trainable
    """
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
            0   : [0, 0, 0, 0, 0, 0], # Begin token
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
            ' ' : [1, 1, 1, 1, 1, 1], 
            1   : [2, 2, 2, 2, 2, 2]  # End token
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
    
class VarCharEncoder(nn.Module):
    def __init__(self, d_q, d_k, embedding_dim=128, num_heads=3):
        super(VarCharEncoder, self).__init__()
        self.var_encoder = VarEncoder(D=6, d_q=d_q, d_k=d_k, embedding_dim=embedding_dim, num_heads=num_heads)
        
    def forward(self, input, output = None):
        z = self.var_encoder(input).squeeze(-1)
        if output is None:
            return z
        else:
            return z, self.embedder(output)
        
class VarCharDecoder(nn.Module):
    def __init__(self, embedding_dim, expanded_dim, dictionary_length, u):
        super(VarCharDecoder, self).__init__()

        self.embedder    = StrEmbedder()  
        self.var_decoder = VarDecoder(embedding_dim, expanded_dim, dictionary_length, u)

    def forward(self, z1, z2):
        y = self.var_decoder(z1, z2)
        return torch.matmul(y, self.embedder.embedding.weight)
    
