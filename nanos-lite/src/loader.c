#include <proc.h>
#include <elf.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
# define ElfN_Addr Elf64_Addr
# define ElfN_Off Elf64_Off
# define ElfN_Half Elf64_Half
# define ElfN_Sword Elf64_Sword
# define ElfN_Word Elf64_Word
# define ElfN_Class ELFCLASS64
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
# define ElfN_Addr Elf32_Addr
# define ElfN_Off Elf32_Off
# define ElfN_Half Elf32_Half
# define ElfN_Sword Elf32_Sword
# define ElfN_Word Elf32_Word
# define ElfN_Class ELFCLASS32
#endif
// 
size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);

bool check_elf(Elf_Ehdr *elf_header)
{
  //only some basic check, including magic number check and arch check, to be added more
  bool magic_check = (elf_header->e_ident[EI_MAG0] == ELFMAG0) && (elf_header->e_ident[EI_MAG1] == ELFMAG1) && (elf_header->e_ident[EI_MAG2] == ELFMAG2) && (elf_header->e_ident[EI_MAG3] == ELFMAG3);
  bool arch_check = (elf_header->e_ident[EI_CLASS] == ElfN_Class);
  return magic_check && arch_check;
}

static uintptr_t loader(PCB *pcb, const char *filename) {
  Elf_Ehdr ehdr;
  //first we read the elf header into an ehdr
  assert(ramdisk_read((void *)&ehdr, 0, sizeof(ehdr)) == sizeof(ehdr));
  assert(check_elf(&ehdr) == true);
  printf("Pass the elf check, success load the elf header!\n");
  //We further process the phdr part, load PT_LOAD segment to its virtual address

  ElfN_Off phoff = ehdr.e_phoff;
  uint32_t phnum = ehdr.e_phnum;
  uint32_t phentsize = ehdr.e_phentsize;
  Elf_Phdr phdr;
  for (uint32_t i = 1 ; i <= phnum ; i++)
  {
    ElfN_Off offset = phoff + (i - 1) * phentsize;
    assert(ramdisk_read((void *)&phdr, offset, sizeof(phdr)) == sizeof(phdr));
    if(phdr.p_type == PT_LOAD) //segments that we need to load!
    {
      ElfN_Off off = phdr.p_offset; // offset of the segment, in the file image
      ElfN_Addr vaddr = phdr.p_vaddr; // the virtual address in memory, this image to be loaded
      ElfN_Word filesz = phdr.p_filesz, memsz = phdr.p_memsz; //get the file size and memory size
      assert(filesz <= memsz);
      assert(ramdisk_read((void *)vaddr, off, filesz) <= memsz);
      memset((void *)(vaddr + filesz), memsz - filesz, 0);
    }
  }
  return ehdr.e_entry; //return the image procedure entry!
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

