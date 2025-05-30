import random

def random_mac():
    return ":".join(f"{random.randint(0, 255):02x}" for _ in range(6))

seen = set()
count = 10000
target_file = f"../server/vlan_mac_{count}.csv"

with open(target_file, "w") as f:
    i = 0
    while i < count:
        vlan_id = random.randint(1, 4094)
        mac = random_mac()
        if (vlan_id, mac) in seen:
            continue
        seen.add((vlan_id, mac))
        f.write(f"{vlan_id},{mac}\n")
        i += 1

print(f"Файл '{target_file}' успешно создан с {count} уникальными записями.")