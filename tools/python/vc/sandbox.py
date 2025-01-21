import torch
import torch.nn.functional as F
from torch.utils.data import Dataset, DataLoader, random_split
import numpy as np
import torch.optim as optim
from varchar import *

e = StrEmbedder()

# batch_strs = ["Hello", "World", "Test", "Example"]

# batch_embeddings = [e(b) for b in batch_strs]

# max_length = max(tensor.shape[0] for tensor in batch_embeddings)
# padded_batch = torch.stack([F.pad(tensor, (0, 0, 0, max_length - tensor.shape[0])) for tensor in batch_embeddings])

# batch = padded_batch

# print(batch.shape)

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
    def __init__(self):
        super(FixedAutoEncoder, self).__init__()
        self.encoder = VarCharEncoder(d_q=32, d_k=32, embedding_dim=128, num_heads=3)
        self.decoder = VarCharDecoder(embedding_dim=128, expanded_dim=4, dictionary_length=128, u=32)

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
        # test with a single string
        pass

    def train(self, num_epochs):
        lossf     = nn.L1Loss()  
        optimizer = optim.Adam(self.model.parameters(), lr=0.001)

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


            print(f'Epoch {epoch+1}, Avg Train Loss: {avg_train_loss:.10f}, Train Change: {train_loss_change:.10f}, stagnant: {stagnant}, Avg Test Loss: {avg_test_loss:.10f}, Test Change: {test_loss_change:.10f}')

autoencoder = FixedAutoEncoder()
trainer = VarCharModelRunner(autoencoder, 'Apache_2k.log')
trainer.train(100)

# e = VarCharEncoder(d_q=32, d_k=32, embedding_dim=128, num_heads=3)
# z = e(batch)

# z = z.unsqueeze(-1)
# print(z.shape)

# d = VarCharDecoder(embedding_dim=128, expanded_dim=4, dictionary_length=128, u=32)
# s = d(z, z)

# print(s.shape)