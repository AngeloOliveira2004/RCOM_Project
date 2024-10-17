def write_numbers_to_file(file_path):
    with open(file_path, 'w') as file:
        for number in range(1, 101):
            file.write(f"{number}\n")

def main():
    file_path = 'teste.txt'
    write_numbers_to_file(file_path)

if __name__ == "__main__":
    main()
