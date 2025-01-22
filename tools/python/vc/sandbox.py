import torch
import torch.nn.functional as F
from torch.utils.data import Dataset, DataLoader, random_split
import numpy as np
import torch.optim as optim
from colorama import Fore, Back, Style

from varchar import *

class LinearStrDataset(Dataset):
    def __init__(self, filepath):
        self.data = []
        with open(filepath, 'r') as file:
            for line in file:
                self.data.append(line.strip())

    def __len__(self):
        return len(self.data)

    def __getitem__(self, idx):
        return self.data[idx]

def PaddedCollator(e: StrEmbedder):

    def collate_batch(batch):
        max_length       = max(len(x) for x in batch)
        batch_embeddings = [e(b) for b in batch]
        padded_batch     = torch.stack([F.pad(tensor, (0, 0, 0, max_length - tensor.shape[0]), value=0) for tensor in batch_embeddings])
        return padded_batch
    
    return collate_batch

class FixedAutoEncoder(nn.Module):
    def __init__(self, dq=32, dk=32, expanded_dim=4, embedding_dim=128, u=32, num_heads=3):
        super(FixedAutoEncoder, self).__init__()
        self.encoder = VarCharEncoder(d_q=dq, d_k=dk, embedding_dim=embedding_dim, num_heads=num_heads)
        self.decoder = VarCharDecoder(embedding_dim=embedding_dim, expanded_dim=expanded_dim, dictionary_length=97, u=u)

        encoder_parameters = sum(p.numel() for p in self.encoder.parameters() if p.requires_grad)
        decoder_parameters = sum(p.numel() for p in self.decoder.parameters() if p.requires_grad)

        print("Parameters: ", encoder_parameters, decoder_parameters)

    def randomize(self):
        self.encoder.randomize()
        self.decoder.randomize()

    def forward(self, x):
        encoded = self.encoder(x)
        encoded = encoded.unsqueeze(-1)  
        decoded = self.decoder(encoded, encoded)
        return decoded
    
class VarCharModelRunner:
    def __init__(self, autoencoder, path, train_ratio=0.8):
        super(VarCharModelRunner, self).__init__()
        embedder        = StrEmbedder()
        dataset         = LinearStrDataset(path)
        collator        = PaddedCollator(embedder)
        self.model      = autoencoder

        self.model.randomize()

        train_size = int(len(dataset) * train_ratio)
        test_size  = len(dataset) - train_size

        self.train_dataset, self.test_dataset = random_split(dataset, [train_size, test_size])

        self.train_dataloader = DataLoader(self.train_dataset, batch_size=32, collate_fn=collator)
        self.test_dataloader  = DataLoader(self.test_dataset,  batch_size=32, collate_fn=collator)

        self.device          = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        self.model.to(self.device)  

        print(f"Training on: {self.device}")
        if self.device.type == 'cuda':
            print(f"GPU Memory Allocated: {torch.cuda.memory_allocated(self.device)} bytes")
            print(f"GPU Memory Reserved: {torch.cuda.memory_reserved(self.device)} bytes")


    def load(self, path):
        self.model.load_state_dict(torch.load(path))

    def test(self, str):
        embedder = StrEmbedder()
        ascii = embedder(str)
        output = self.model(ascii.unsqueeze(0))  
        out_str = embedder.decode_str(output)
        return out_str

    def train(self, num_epochs):
        embedder  = StrEmbedder()
        lossf     = nn.MSELoss()  
        optimizer = optim.SGD(self.model.parameters(), lr=0.001)

        least_train_loss = math.inf 
        least_test_loss  = math.inf
        stagnant   = 0

        for epoch in range(num_epochs):
            self.model.train()
            train_losses = []
            for batch in self.train_dataloader:
                batch = batch.to(self.device)
                optimizer.zero_grad()
                output = self.model(batch.float())  
                max_length = output.size(1)
                padded_batch = F.pad(batch, (0, 0, 0, max_length - batch.size(1)), 'constant', value=0)
                loss   = lossf(output, padded_batch.float())  
                train_losses.append(loss.item())
                loss.backward()
                optimizer.step()

            avg_train_loss = sum(train_losses)/len(train_losses)
            train_loss_change = avg_train_loss - least_train_loss
            self.model.eval() 

            with torch.no_grad():
                test_losses = []
                for batch in self.test_dataloader:
                    batch = batch.to(self.device)
                    output = self.model(batch.float())
                    max_length = output.size(1)
                    padded_batch = F.pad(batch, (0, 0, 0, max_length - batch.size(1)), 'constant', value=0)
                    loss = lossf(output, padded_batch.float())
                    test_losses.append(loss.item())
            
            avg_test_loss = sum(test_losses) / len(test_losses)
            test_loss_change = avg_test_loss - least_test_loss

            if avg_train_loss < least_train_loss:
                least_train_loss = avg_train_loss
                stagnant = 0
            else:
                stagnant = stagnant +1

            if avg_test_loss < least_test_loss:
                least_test_loss = avg_test_loss
                torch.save(self.model.state_dict(), f'wp45linear-l{least_train_loss:.4f}.pth')

            if stagnant > 200:
                break

            if train_loss_change < 0:
                print(Fore.GREEN + f'Epoch {epoch+1}, Avg Train Loss: {avg_train_loss:.10f}, Train Change: {train_loss_change:.10f}, stagnant: {stagnant}, Avg Test Loss: {avg_test_loss:.10f}, Test Change: {test_loss_change:.10f}')
            elif test_loss_change < 0:
                print(Fore.BLUE + f'Epoch {epoch+1}, Avg Train Loss: {avg_train_loss:.10f}, Train Change: {train_loss_change:.10f}, stagnant: {stagnant}, Avg Test Loss: {avg_test_loss:.10f}, Test Change: {test_loss_change:.10f}')
            else:
                print(Fore.RED + f'Epoch {epoch+1}, Avg Train Loss: {avg_train_loss:.10f}, Train Change: {train_loss_change:.10f}, stagnant: {stagnant}, Avg Test Loss: {avg_test_loss:.10f}, Test Change: {test_loss_change:.10f}')

    print(Style.RESET_ALL)

autoencoder = FixedAutoEncoder(dq=65, dk=34, expanded_dim=16, embedding_dim=128, u=64, num_heads=8)
trainer = VarCharModelRunner(autoencoder, 'Apache_2k.log')
# trainer.load("wp45linear-l0.4401.pth")
trainer.train(1000)
output = trainer.test("[Sun Dec 04 04:51:18 2005] [error] mod_jk child workerEnv in error state 6")
print(output)

# e = StrEmbedder()
# batch_strs       = ["Hello", "World", "Test", "Example"]
# batch_embeddings = [e(b) for b in batch_strs]
# max_length       = max(tensor.shape[0] for tensor in batch_embeddings)
# padded_batch     = torch.stack([F.pad(tensor, (0, 0, 0, max_length - tensor.shape[0])) for tensor in batch_embeddings])
# expected_output  = torch.stack([F.pad(tensor, (0, 0, 0, e.num_embeddings - tensor.shape[0])) for tensor in batch_embeddings])

# print("padded batch", padded_batch.shape)
# ven = VarCharEncoder(d_q=32, d_k=32, embedding_dim=128, num_heads=3)
# z = ven(padded_batch)
# print("z", z.shape)
# z = z.unsqueeze(-1)
# vde = VarCharDecoder(embedding_dim=128, expanded_dim=4, dictionary_length=97, u=32)
# s = vde(z, z)
# print("output", s.shape)
# print("expected output", expected_output.shape)
# print(padded_batch[0])
# print(expected_output[0])