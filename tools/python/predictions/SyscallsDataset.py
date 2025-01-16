import os
import json
import torch
import mmh3
import torch
import numbers 
from torch.utils.data import Dataset
import torch.nn.functional as F

class SyscallsDataset(Dataset):
    def __init__(self, directory_path):
        super(SyscallsDataset, self).__init__()
        self.data = []
        self._fixed_keys = set()
        self._load_data(directory_path)

    def _load_data(self, directory_path):
        exu_data      = []
        for filename in os.listdir(directory_path):
            if filename.endswith('.syscalls.json'):
                full_path = os.path.join(directory_path, filename)
                with open(full_path, 'r') as file:
                    print("Reading ", full_path)
                    json_data = json.load(file)
                    exu_data.append(json_data)
                    for item in json_data:
                        self._prepare(item)
                        
        self.adjust_keys()

        print("exu ", len(exu_data))
        counter = 1
        for j in exu_data:
            attributes = []
            for e in j:
                attributes.append(self._process_item(e))
            print(counter)
            counter = counter +1
            self.data.append(attributes)

    def _add_key(self, key):
        if key not in self._fixed_keys:
            self._fixed_keys.add(key)
    
    def adjust_keys(self):
        # sort the fixed keys by value and once the 
        unique_keys = list(self._fixed_keys)  # or however you track them
        unique_keys.sort()
        new_keys = {k: i for i, k in enumerate(unique_keys)}
        self._fixed_keys = new_keys

    def fixed_keys(self):
        return self._fixed_keys
    
    def fixed_key(self, key):
        return self._fixed_keys[key]

    def _prepare(self, item):
        for key, value in item.items():
            self._add_key(key)
            if 'path' not in key and 'exe' not in key:
                if not isinstance(value, numbers.Number):
                    self._add_key(value)

    def _process_item(self, item):
        processed_item = []
        for key, value in item.items():
            vtype = None
            if 'path' in key or 'exe' in key:
                vtype = "path"
            else:
                if isinstance(value, numbers.Number):
                    vtype = "numeric"
                else:
                    vtype = "word"
                    value = self.fixed_key(value),

            processed_item.append({
                "k": self.fixed_key(key),
                "t": vtype,
                "v": value
            })

        return processed_item

    def __len__(self):
        return len(self.data)

    def __getitem__(self, idx):
        return self.data[idx]
    