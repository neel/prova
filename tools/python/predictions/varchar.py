import torch
import torch.nn as nn
import torch.nn.functional as F
import math 

class NormalizedProjector(nn.Module):
    def __init__(self):
        super(NormalizedProjector, self).__init__()
    def forward(self, x, w):
        assert x.shape[1] == w.shape[0], "The dimensions of x and w are incompatible for matrix multiplication."

        scale = torch.sqrt(torch.tensor(w.shape[0], dtype=torch.float32))

        x = torch.matmul(x, w)
        x = x / scale
        x = F.softmax(x, dim=1)
        return x

class GenericAttention(nn.Module):
    def __init__(self):
        super(GenericAttention, self).__init__()
        self.projector = NormalizedProjector()

    def forward(self, Q, K):
        assert Q.shape[1] == K.shape[1], "The dimensions of Q and K are incompatible in the second dimension."

        return self.projector(Q, torch.transpose(K, 0, 1))
    
class GenericCoAttention(nn.Module):
    def __init__(self):
        super(GenericCoAttention, self).__init__()
        self.projector = NormalizedProjector()

    def forward(self, Q, K):
        assert Q.shape[0] == K.shape[0], "The dimensions of Q and K are incompatible in the first dimension."

        return self.projector(torch.transpose(Q, 0, 1), K)
    
class Attention(nn.Module):
    def __init__(self, D, u):
        super(Attention, self).__init__()
        self.D = D 
        self.u = u 
        self._attention = GenericAttention()
        self._phi = nn.Parameter(torch.zeros(D, u))
        self._psi = nn.Parameter(torch.zeros(D, u))

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

    def forward(self, x):
        assert x.shape[1] == self.D, ("X must be of the shape [L, {D}] for some L, but it is of the shape [{L}, {D}]").format(L=x.shape[0], D=self.D) 

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

    def forward(self, h1, h2):
        assert h1.shape[0] == h2.shape[0], "Both h1 and h2 must have same size in the first dimension"
        assert h1.shape[1] == self.v, "Incompatible h1, it must be of the shape [_, {}]".format(self.v)
        assert h2.shape[1] == self.v, "Incompatible h2, it must be of the shape [_, {}]".format(self.v)

        h1 = torch.matmul(h1, self._omega)
        h2 = torch.matmul(h2, self._zeta)
        h  = torch.matmul(torch.transpose(h1, 0, 1), h2)
        return h 
    
class FeatureCrossover(nn.Module):
    def __init__(self, eta, zeta, u):
        super(FeatureCrossover, self).__init__()
        self.eta  = eta
        self.zeta = zeta 
        self.u    = u

        self._phi = nn.Parameter(torch.zeros(zeta, u))
        self._psi = nn.Parameter(torch.zeros(eta, u))

        self._projector = NormalizedProjector()
        self._attention = GenericAttention()

    def forward(self, m1, m2):
        assert m1.shape == [self.eta, self.zeta], "Incompatible m1, it must be of the shape [{}, {}]".format(self.eta, self.zeta)
        assert m2.shape == [self.eta, self.zeta], "Incompatible m2, it must be of the shape [{}, {}]".format(self.eta, self.zeta)

        hm1 = self._projector(m1, self._phi)
        hm2 = self._projector(torch.transpose(m2, 0, 1), self._psi)
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

    def forward(self, z1, z2):
        assert z1.shape == [self.eta, self.tau], "Incompatible m1, it must be of the shape [{}, {}]".format(self.eta, self.tau)
        assert z2.shape == [self.eta, self.tau], "Incompatible m2, it must be of the shape [{}, {}]".format(self.eta, self.tau)

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
        Zg = F.gumbel_softmax(Ar, 0.08, dim=1, hard=False)
        return Zg
    
class Enricher(nn.Module):
    def __init__(self, tau):
        super(Enricher, self).__init__()
        self.tau = tau 

        self._beta = nn.Parameter(torch.zeros(1, tau))
        self._s    = nn.Parameter(torch.zeros(1, tau))
        self._mu   = nn.Parameter(torch.zeros(2*tau, tau))

    def forward(self, z):
        z_beta = torch.matmul(z, self._beta)
        z_s    = torch.matmul(z, self._s)
        z_s    = F.softmax(z_s, dim=1)
        cat    = torch.cat([z_beta, F.relu(z_s)], dim=1)
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
        pos_encoding = self.pe[:x.size(0)].detach()
        x = x + pos_encoding
        return x
    
class VarEncoder(nn.Module):
    def __init__(self, num_heads, D, d_q, d_k, embedding_dim):
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
        assert x.shape[1] == self.D

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
    