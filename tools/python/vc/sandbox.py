import os
import sys
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

class FixedAutoEncoder(nn.Module):
    def __init__(self, dq=32, dk=32, expanded_dim=4, embedding_dim=128, u=32, num_heads=3):
        super(FixedAutoEncoder, self).__init__()
        self.encoder  = VarCharEncoder(d_q=dq, d_k=dk, embedding_dim=embedding_dim, num_heads=num_heads)
        self.embedder = StrEmbedder()
        self.decoder  = VarCharDecoder(embedding_dim=embedding_dim, expanded_dim=expanded_dim, dictionary_length=97, u=u)

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
        return decoded, encoded
    
class VarCharModelRunner:
    def __init__(self, autoencoder, path, train_ratio=0.8):
        super(VarCharModelRunner, self).__init__()
        self.model      = autoencoder
        dataset         = LinearStrDataset(path)

        # self.model.randomize()

        self.device          = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        self.model.to(self.device)  

        train_size = int(len(dataset) * train_ratio)
        test_size  = len(dataset) - train_size

        self.train_dataset, self.test_dataset = random_split(dataset, [train_size, test_size])

        self.train_dataloader = DataLoader(self.train_dataset, batch_size=32, collate_fn=padded_collator)
        self.test_dataloader  = DataLoader(self.test_dataset,  batch_size=32, collate_fn=padded_collator)

        print(f"Training on: {self.device}")
        if self.device.type == 'cuda':
            print(f"GPU Memory Allocated: {torch.cuda.memory_allocated(self.device)} bytes")
            print(f"GPU Memory Reserved: {torch.cuda.memory_reserved(self.device)} bytes")


    def load(self, path):
        self.model.load_state_dict(torch.load(path))

    def test(self, txt):
        self.model.eval()
        collator   = PaddedCollator(self.model.embedder, self.device)
        inputs, selections = collator([txt])
        output, z = self.model(inputs)
        out_indices = self.model.embedder.decode_selection(output)
        out_str = self.model.embedder.decode_str(out_indices)
        print(z.squeeze(0).squeeze(1))
        return out_str

    def lossf(self, output, target):
        dist_mat  = torch.cdist(output, target)**2
        dist_diag = torch.diag(dist_mat)
        return torch.sum(dist_diag)

    def train(self, num_epochs):
        # lossf = nn.BCEWithLogitsLoss()
        lossf = nn.MSELoss()
        # optimizer = optim.SGD(self.model.parameters(), lr=0.001)
        optimizer = optim.Adam(self.model.parameters(), lr=1e-5)

        least_train_loss = math.inf 
        least_test_loss  = math.inf
        stagnant   = 0

        for epoch in range(num_epochs):
            self.model.train()
            train_losses = []
            for batch in self.train_dataloader:
                inputs      = batch["variations"].to(self.device)
                selections  = batch["selections"].to(self.device)

                optimizer.zero_grad()
                output, z = self.model(inputs)  
                z = z.squeeze(-1)
                
                # print("<>", output.shape, batch.shape)

                diff = output.size(1) - selections.size(1)
                if diff < 0:
                    output = F.pad(output, (0, 0, 0, -diff), 'constant', value=0)
                else:
                    selections = F.pad(selections, (0, 0, 0, diff), 'constant', value=0)
                expected = selections
                # print("<>", output.shape, expected.shape)

                reconstruction_loss = lossf(output.float(), expected.float())
                # latent_variance = 1/torch.var(z, dim=0).mean()

                loss   = reconstruction_loss #+ latent_variance

                train_losses.append(loss.item())
                loss.backward()
                optimizer.step()

            avg_train_loss = sum(train_losses)/len(train_losses)
            train_loss_change = avg_train_loss - least_train_loss
            self.model.eval() 

            with torch.no_grad():
                test_losses = []
                for batch in self.test_dataloader:
                    inputs      = batch["variations"].to(self.device)
                    selections  = batch["selections"].to(self.device)

                    optimizer.zero_grad()
                    output, z = self.model(inputs)  
                    z = z.squeeze(-1)
                    
                    # print("<>", output.shape, batch.shape)

                    diff = output.size(1) - selections.size(1)
                    if diff < 0:
                        output = F.pad(output, (0, 0, 0, -diff), 'constant', value=0)
                    else:
                        selections = F.pad(selections, (0, 0, 0, diff), 'constant', value=0)
                    expected = selections
                    # print("<>", output.shape, expected.shape)

                    reconstruction_loss = lossf(output.float(), expected.float())
                    # latent_variance = 1/torch.var(z, dim=0).mean()

                    loss   = reconstruction_loss #+ latent_variance

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

            if stagnant > 1000:
                break

            if train_loss_change < 0:
                print(Fore.GREEN + f'Epoch {epoch+1}, Avg Train Loss: {avg_train_loss:.10f}, Train Change: {train_loss_change:.10f}, stagnant: {stagnant}, Avg Test Loss: {avg_test_loss:.10f}, Test Change: {test_loss_change:.10f}')
            elif test_loss_change < 0:
                print(Fore.BLUE + f'Epoch {epoch+1}, Avg Train Loss: {avg_train_loss:.10f}, Train Change: {train_loss_change:.10f}, stagnant: {stagnant}, Avg Test Loss: {avg_test_loss:.10f}, Test Change: {test_loss_change:.10f}')
            else:
                print(Fore.RED + f'Epoch {epoch+1}, Avg Train Loss: {avg_train_loss:.10f}, Train Change: {train_loss_change:.10f}, stagnant: {stagnant}, Avg Test Loss: {avg_test_loss:.10f}, Test Change: {test_loss_change:.10f}')

    print(Style.RESET_ALL)

def train():
    autoencoder = FixedAutoEncoder(dq=65, dk=34, expanded_dim=16, embedding_dim=128, u=64, num_heads=8)
    autoencoder.randomize()
    trainer = VarCharModelRunner(autoencoder, 'Apache_2k.log')
    trainer.train(50000)

def test():
    checkpoint_files = glob.glob('wp45linear-*.pth')
    checkpoint_files.sort(key=os.path.getmtime, reverse=True)
    for checkpoint in checkpoint_files:
        autoencoder = FixedAutoEncoder(dq=65, dk=34, expanded_dim=16, embedding_dim=128, u=64, num_heads=8)
        trainer = VarCharModelRunner(autoencoder, 'Apache_2k.log')
        trainer.load(checkpoint)
        output = trainer.test("[Sun Dec 04 04:51:18 2005] [error] mod_jk child workerEnv in error state 6")
        print(checkpoint, output)

if len(sys.argv) < 2:
    print("Usage: python script_name.py train|test [additional-arguments]")
    sys.exit(1)
    
mode = sys.argv[1].lower()

if mode == 'train':
    train()
elif mode == 'test':
    test()

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