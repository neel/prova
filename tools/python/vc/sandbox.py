import os
import glob
import torch
import torch.nn.functional as F
from torch.utils.data import Dataset, DataLoader, random_split
import numpy as np
import torch.optim as optim
from colorama import Fore, Back, Style
import random
from varchar import *

torch.manual_seed(0)
np.random.seed(0)
random.seed(0)
if torch.cuda.is_available():
    torch.cuda.manual_seed_all(0)

torch.backends.cudnn.deterministic = True
torch.backends.cudnn.benchmark = False

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

def PaddedCollator():

    def collate_batch(batch):
        max_length       = max(len(x) for x in batch)
        batch_embeddings = [torch.tensor([95-(ord(c)-32) for c in b]) for b in batch]
        expected     = torch.stack([F.pad(tensor, (0, max_length - tensor.shape[0]), value=0) for tensor in batch_embeddings])
        return expected.to(torch.int32)
    
    return collate_batch

class FixedAutoEncoder(nn.Module):
    def __init__(self, dq=32, dk=32, expanded_dim=4, embedding_dim=128, u=32, num_heads=3):
        super(FixedAutoEncoder, self).__init__()
        self.encoder  = VarCharEncoder(d_q=dq, d_k=dk, embedding_dim=embedding_dim, num_heads=num_heads)
        self.embedder = self.encoder.embedder
        self.decoder  = VarCharDecoder(embedder=self.embedder, embedding_dim=embedding_dim, expanded_dim=expanded_dim, dictionary_length=97, u=u)

        encoder_parameters = sum(p.numel() for p in self.encoder.parameters() if p.requires_grad)
        decoder_parameters = sum(p.numel() for p in self.decoder.parameters() if p.requires_grad)

        print("Parameters: ", encoder_parameters, decoder_parameters)

    def randomize(self):
        self.encoder.randomize()
        self.decoder.randomize()

    def forward(self, x):
        encoded, embedded = self.encoder(x, x)
        encoded = encoded.unsqueeze(-1)  
        decoded = self.decoder(encoded, encoded)
        return embedded, decoded
    
class VarCharModelRunner:
    def __init__(self, autoencoder, path, train_ratio=0.8):
        super(VarCharModelRunner, self).__init__()
        self.model      = autoencoder
        dataset         = LinearStrDataset(path)
        collator        = PaddedCollator()

        # self.model.randomize()

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

    def test(self, input):
        self.model.eval()
        ascii_values = torch.tensor([95-(ord(c)-32) for c in input])
        ascii_values = ascii_values.to(self.device).unsqueeze(0)
        embedded, output = self.model(ascii_values)
        output = output.squeeze(0)
        out_chars = [chr(int(95-o)+32) for o in output]
        return "".join(out_chars)

    def lossf(self, output, target):
        dist_mat  = torch.cdist(output, target)**2
        dist_diag = torch.diag(dist_mat)
        return torch.sum(dist_diag)

    def train(self, num_epochs):
        # optimizer = optim.SGD(self.model.parameters(), lr=0.001)
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
                embedded, output = self.model(batch)  

                # print("<>", output.shape, batch.shape)

                diff = output.size(1) - batch.size(1)
                if diff < 0:
                    output = F.pad(output, (0, -diff), 'constant', value=0)
                else:
                    batch = F.pad(batch, (0, diff), 'constant', value=0)
                expected = batch
                # print("<>", output.shape, expected.shape)

                loss   = self.lossf(output.float(), expected.float())
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
                    embedded, output = self.model(batch)  

                    # print("<>", output.shape, batch.shape)

                    diff = output.size(1) - batch.size(1)
                    if diff < 0:
                        output = F.pad(output, (0, -diff), 'constant', value=0)
                    else:
                        batch = F.pad(batch, (0, diff), 'constant', value=0)
                    expected = batch
                    # print("<>", output.shape, expected.shape)

                    loss   = self.lossf(output.float(), expected.float())  
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

                checkpoint_files = glob.glob('wp45linear-*.pth')
                checkpoint_files.sort(key=os.path.getmtime, reverse=True)
                for old_file in checkpoint_files[5:]:
                    os.remove(old_file)

            if stagnant > 100:
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
trainer.load("wp45linear-l5608724.3400.pth")
trainer.train(50000)
output = trainer.test("[Sun Dec 04 04:52:05 2005] [notice] jk2_init() Found child 6737 in scoreboard slot 8")
print(output)

# e = StrEmbedder()
# batch_strs       = ["Hello", "World", "Test", "Example"]
# batch_embeddings = [e(b) for b in batch_strs]
# max_length       = max(tensor.shape[0] for tensor in batch_embeddings)
# expected     = torch.stack([F.pad(tensor, (0, 0, 0, max_length - tensor.shape[0])) for tensor in batch_embeddings])
# expected_output  = torch.stack([F.pad(tensor, (0, 0, 0, e.num_embeddings - tensor.shape[0])) for tensor in batch_embeddings])

# print("padded batch", expected.shape)
# ven = VarCharEncoder(d_q=32, d_k=32, embedding_dim=128, num_heads=3)
# z = ven(expected)
# print("z", z.shape)
# z = z.unsqueeze(-1)
# vde = VarCharDecoder(embedding_dim=128, expanded_dim=4, dictionary_length=97, u=32)
# s = vde(z, z)
# print("output", s.shape)
# print("expected output", expected_output.shape)
# print(expected[0])
# print(expected_output[0])