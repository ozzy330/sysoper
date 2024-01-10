#include "./nachostablita.h"

NachosOpenFilesTable::NachosOpenFilesTable() {
  this->openFiles = new int[MaxOpenFiles];
  this->openFilesMap = new BitMap(MaxOpenFiles);
  this->usage = 0;
}

NachosOpenFilesTable::~NachosOpenFilesTable() {
  delete this->openFiles;
  delete this->openFilesMap;
}
int NachosOpenFilesTable::Open(int UnixHandle) {
  int position = this->openFilesMap->Find();
  this->openFiles[position] = UnixHandle;
  return position;
}
int NachosOpenFilesTable::Close(int NachosHandle) {
  this->openFilesMap->Clear(NachosHandle);
  return this->openFiles[NachosHandle];
}
bool NachosOpenFilesTable::isOpened(int NachosHandle) {
  return this->openFilesMap->Test(NachosHandle);
}
int NachosOpenFilesTable::getUnixHandle(int NachosHandle) {
  return this->openFiles[NachosHandle];
}
void NachosOpenFilesTable::addThread() { usage++; }
void NachosOpenFilesTable::delThread() { usage--; }
void NachosOpenFilesTable::Print() {
  int cantOpenFile = this->openFilesMap->NumClear() - MaxOpenFiles;
  for (int file = 0; file < cantOpenFile; file++) {
    printf("UnixHandle: %i |  NachOsHandle %i | Open: %s",
           this->openFiles[file], file,
           this->openFilesMap->Test(file) ? "true" : "false");
  }
}
