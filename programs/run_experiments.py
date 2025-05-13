import os

if __name__ == "__main__":
    N = 1080 
    client_nums = [4, 6, 8, 10, 12, 15, 20, 30, 40, 60]
    for client_n in client_nums:
        os.system(f"./a.out {N} {client_n} 100")

