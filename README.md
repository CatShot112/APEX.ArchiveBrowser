# APEX.ArchiveBrowser
# WARNING: Still W.I.P. Stuff may be broken or not working.

## Info
Simple program for browsing and extracting APEX Engine archive files (tested on theHunter: Call of the Wild, Generation Zero).  

Currently you will need to compile the program by yourself.  
I will add build actions on 1.0.0 release (no ETA, will release when I'm happy with source :D).

## Usage
1. Initialize database (if not initialized) (Database -> Initialize):  
   Will create .db file with props and vPaths.  
   You can skip creating database if loading data from text file.  
2. Load data into memory (Database -> Load from .txt/.db file).  
3. Open single archive or whole folder (File -> Open File/Folder):  
   If opening folder program will load all files recursively.  
   You can select .arc or .tab.  
5. If you have duplicated hashes in database/memory you can select which vPath should be used.  
   File -> Resolve duplicates.  
   You can extract file to view it in external program (hex editor or something).  
   If you can't see correct vPath then add it to VPaths.txt

If database file has been corrupted just delete it and initialize again.  
Loading data from database should be slighty faster than from text files.

## Compilation
1. Open .sln and pray that everything works out of the box :D
2. You will need to download SDL3.dll from [HERE](https://github.com/libsdl-org/SDL/releases/tag/release-3.4.0) (SDL3-3.4.0-win32-x64.zip) and put it alongside compiled .exe.
3. Put Props.txt and VPaths.txt alongside compiled .exe.
