Arquivos de Ferramentas:

c:\Users\rinal\Documents\mkfatimg - Copia\Release>mkfatimg.exe c:\Users\rinal\STM32CubeIDE\workspace_1.3.0\EX-XT\Sinteck\img\ SPI_EX_XT-191.bin 65536 512


Linux:

dd if=/dev/zero of=fatfs.img bs=1M count=64
mkfs.fat -F 32 -n  FATFS_XT fatfs.img

mkdir /mnt/fatfs
sudo mount -o loop fatfs.img /mnt/fatfs
sudo cp -R "Folder IMG" /mnt/fatfs/.
sudo umount /mnt/fatfs
