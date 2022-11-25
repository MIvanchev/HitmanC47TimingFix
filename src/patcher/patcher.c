/*
This file is part of HitmanC47TimingFix licensed under the BSD 3-Clause License
the full text of which is given below.

Copyright (c) 2022-present, Mihail Ivanchev
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <windows.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include "md5.h"
#include "miniz.h"
#include "patches.h"

#define BUFLEN(buf) (sizeof((buf)) / sizeof((buf)[0]))
#define MIN(valA, valB) (((valA) <= (valB)) ? (valA) : (valB))
#define MAX(valA, valB) (((valA) >= (valB)) ? (valA) : (valB))

#define TEXT_RUN_AGAIN "Try running the patcher again."
#define TEXT_CLOSE_PROGS_AND_RUN_AGAIN "Close some programs and try running " \
                                       "the patcher again."
#define TEXT_DELETE_AND_RUN_AGAIN "In order to run the patcher successfully " \
                                  "you must restore the original game files " \
                                  "and delete the backup file. A new backup " \
                                  "file will be created by the patcher " \
                                  "before it modifies any files."
#define TEXT_INCONSISTENT_BACKUP "An error occured while writing the " \
                                 "backup file %s. This file now has an " \
                                 "inconsistent state and you need " \
                                 "to delete it before rerunning the patcher."
#define TEXT_PLEASE_REPORT "Please report this error."

/*#define VALIDATE_CHECKSUMS*/

struct Section {
    const char *name;
    DWORD virtualAddr;
    DWORD size;
    DWORD rawDataPtr;
};

struct PatchedFile {
    const char *name;
    const char *nameUppercase;
    const char *namePrefix;
    const size_t size;
    const DWORD imageBase;
    const char *md5Unpatched;
    const char *md5Patched;
    const struct Section sections[4];
    char *nameInFilesystemCase;
    int handleOpened;
    HANDLE handle;
    BOOL alreadyPatched;
    void *data;
    DWORD lastChangeAddr;
};

struct PatchedFile patchedFiles[] = {
    {
        "system.dll",
        "SYSTEM.DLL",
        "system",
        278528,
        0x0FFA0000,
        "90b1ac786841bdd5f45077b97fdb83ee",
        "904228e19a6fc01d21167d3202bd89b2",
        {   { ".text", 0x1000, 0x32000, 0x1000 },
            { ".rdata", 0x33000, 0x5000, 0x33000 },
            { ".data", 0x38000, 0x8000, 0x38000 },
            { NULL, 0 }
        }
    },
    {
        "EngineData.dll",
        "ENGINEDATA.DLL",
        "enginedata",
        249856,
        0x0FF60000,
        "878acb8049a6c513f6bdce4bb4f6c92c",
        "878acb8049a6c513f6bdce4bb4f6c92c",
        {   { ".text", 0x1000, 0x2f000, 0x1000 },
            { ".rdata", 0x30000, 0x3000, 0x30000 },
            { ".data", 0x33000, 0x6000, 0x33000 },
            { NULL, 0 }
        }
    },
    {
        "HitmanDlc.dlc",
        "HITMANDLC.DLC",
        "hitmandlc",
        2555904,
        0x0FCC0000,
        "bf3e32ba24d2816adb8ba708774f1e1a",
        "bf3e32ba24d2816adb8ba708774f1e1a",
        {   { ".text", 0x1000, 0x1ef000, 0x1000 },
            { ".rdata", 0x1f0000, 0x2f000, 0x1f0000 },
            { ".data", 0x21f000, 0x25000, 0x21f000 },
            { NULL, 0 }
        },
    },
    { NULL }
};

static const char *bkpFilename = "patch-backup.zip";
static const char *bkpFilenameUppercase = "PATCH-BACKUP.ZIP";
static HANDLE bkpHandle;
static BOOL allFilesPatched = TRUE;

static void showMsg(int type, const char *msg, ...)
{
    va_list args;
    char buf[512];

    assert(BUFLEN(buf) > 256);
    assert(msg);

    va_start(args, msg);
    vsnprintf(buf, BUFLEN(buf), msg, args);
    va_end(args);

    MessageBox(NULL, buf, "Patcher", type);
}

#define showInfo(...) showMsg(MB_OK | MB_ICONINFORMATION, __VA_ARGS__)
#define showError(...) showMsg(MB_OK | MB_ICONERROR, \
                               "Patching failed. " __VA_ARGS__)

static int hexDigitToNum(char digit)
{
    int res;

    res = -1;

    switch (digit) {
    case '0':
        res = 0;
        break;
    case '1':
        res = 1;
        break;
    case '2':
        res = 2;
        break;
    case '3':
        res = 3;
        break;
    case '4':
        res = 4;
        break;
    case '5':
        res = 5;
        break;
    case '6':
        res = 6;
        break;
    case '7':
        res = 7;
        break;
    case '8':
        res = 8;
        break;
    case '9':
        res = 9;
        break;
    case 'A':
    case 'a':
        res = 10;
        break;
    case 'B':
    case 'b':
        res = 11;
        break;
    case 'C':
    case 'c':
        res = 12;
        break;
    case 'D':
    case 'd':
        res = 13;
        break;
    case 'E':
    case 'e':
        res = 14;
        break;
    case 'F':
    case 'f':
        res = 15;
        break;
    }

    return res;
}

static void getMd5Checksum(void *data, DWORD size, char *checksum)
{
    MD5Context ctx;
    size_t ii;

    assert(data);
    assert(size);
    assert(size < SIZE_MAX);
    assert(checksum);

    md5Init(&ctx);
    md5Update(&ctx, data, size);
    md5Finalize(&ctx);

    for (ii = 0; ii < 16; ii++)  {
        sprintf(&checksum[2*ii], "%02x", (unsigned int) ctx.digest[ii]);
    }
}

static void closeFiles(void)
{
    struct PatchedFile *file;

    for (file = patchedFiles; file->name; file++) {
        if (file->handleOpened) {
            CloseHandle(file->handle);
            file->handleOpened = FALSE;
        }
        if (file->data) {
            free(file->data);
            file->data = NULL;
        }
        if (file->nameInFilesystemCase) {
            free(file->nameInFilesystemCase);
            file->nameInFilesystemCase = NULL;
        }
    }
}

static int readFile(struct PatchedFile *file)
{
    size_t nameSizeInBytes;
    char *nameInFilesystemCase;
    HANDLE findHandle;
    BOOL findHandleOpened;
    WIN32_FIND_DATAA findData;
    HANDLE handle;
    size_t sizeInBytes;
    void *data;
    DWORD bytesRead;
    char checksum[33];
    BOOL alreadyPatched;

    alreadyPatched = FALSE;
    findHandleOpened = FALSE;
    nameInFilesystemCase = NULL;
    data = NULL;

    assert(file);
    assert(!alreadyPatched);
    assert(!findHandleOpened);
    assert(!nameInFilesystemCase);
    assert(!data);

    findHandle = FindFirstFile(file->name, &findData);
    if (findHandle == INVALID_HANDLE_VALUE) {
        showError("Failed to query the file system name "
                  "of file %s. Make sure the patcher is located in the "
                  "game's main directory containing HITMAN.EXE and run "
                  "the patcher again.", file->nameUppercase);
        goto fail_find_file;
    }

    findHandleOpened = TRUE;

    if (_stricmp(file->name, findData.cFileName)) {
        showError("Failed to query the file system name of "
                  "file %s. The query returned the result %s. This is "
                  "likely a bug in the patcher, please report it.",
                  file->nameUppercase, findData.cFileName);
        goto fail_invalid_fs_case;
    }

    /* FIXME: Proper undefined behavior checks. */

    nameSizeInBytes = strlen(file->name) + 1;
    nameInFilesystemCase = malloc(nameSizeInBytes);
    if (nameInFilesystemCase == NULL) {
        showError("Couldn't allocate %zu bytes of memory. "
                  TEXT_CLOSE_PROGS_AND_RUN_AGAIN,
                  nameSizeInBytes);
        goto fail_alloc_fs_case;
    }

    strcpy(nameInFilesystemCase, findData.cFileName);

    FindClose(findHandle);
    findHandleOpened = FALSE;

    /* Read file to be patched and verify the checksum. */

    handle = CreateFile(file->name, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        showError("Couldn't open the file %s. Make sure the "
                  "file is not being used by another application and the "
                  "patcher is located in the game's main directory containing "
                  "HITMAN.EXE.",
                  file->nameUppercase);
        goto fail_open;
    }

    /* Allocate an additional byte of space so we can test for the expected
    ** file size.
    */

    sizeInBytes = file->size + 1;
    if (!(data = malloc(sizeInBytes))) {
        showError("Couldn't allocate %zu bytes of memory. "
                  TEXT_CLOSE_PROGS_AND_RUN_AGAIN,
                  sizeInBytes);
        goto fail_alloc_data;
    }

    if (!ReadFile(handle, data, file->size + 1, &bytesRead, NULL)) {
        showError("Couldn't read the contents of the file %s.",
                  file->nameUppercase);
        goto fail_read;
    }

    if (bytesRead != file->size) {
        showError("The file %s has an unexpected file size of "
                  "%lu bytes, but it should be exactly %lu bytes. This could "
                  "be caused by a failed prior patching attempt, the "
                  "application of another patch or an supported game version.",
                  file->nameUppercase,
                  (unsigned long) bytesRead,
                  (unsigned long) file->size);
        goto fail_invalid_size;
    }


    getMd5Checksum(data, file->size, checksum);

    if (strcmp(checksum, file->md5Unpatched)) {
        if (strcmp(checksum, file->md5Patched)) {
#ifdef VALIDATE_CHECKSUMS
            showError("The MD5 checksum of file %s is\n\n"
                      "%s\n\nbut it should be\n\n%s\n\nThis could be caused "
                      "by a failed prior patching attempt, the application "
                      "of another patch or an unsupported game version.",
                      file->nameUppercase,
                      checksum, file->md5Unpatched);
            goto fail_invalid_checksum;
#endif
        }
        else {
            /* Not necessarily an error yet, remember the state. */
            alreadyPatched = 1;
        }
    }

    if (SetFilePointer(handle, 0, NULL,
                       FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        showError("Patching failed. Couldn't prepare the file %s for "
                  "writing after having read and verified its contents. "
                  TEXT_RUN_AGAIN, file->nameUppercase);
        goto fail_rewind;
    }

    file->data = data;
    file->nameInFilesystemCase = nameInFilesystemCase;
    file->handle = handle;
    file->handleOpened = TRUE;
    file->alreadyPatched = alreadyPatched;

    allFilesPatched = file->alreadyPatched ? allFilesPatched : FALSE;

    return 1;

fail_rewind:
fail_invalid_checksum:
fail_invalid_size:
fail_read:
    if (data) {
        free(data);
    }
fail_open:
    CloseHandle(handle);
fail_alloc_data:
    if (nameInFilesystemCase) {
        free(nameInFilesystemCase);
    }
fail_alloc_fs_case:
fail_invalid_fs_case:
    if (findHandleOpened) {
        FindClose(findHandle);
    }
fail_find_file:
    return 0;
}

static int writeFile(struct PatchedFile *file)
{
    char checksum[33];

    assert(file);

    getMd5Checksum(file->data, file->size, checksum);

    if (strcmp(checksum, file->md5Patched)) {
#ifdef VALIDATE_CHECKSUMS
        showError("The patched MD5 checksum of file %s is\n\n%s\n\nbut should "
                  "be\n\n%s\n\nThis is very likely an internal patcher "
                  "error. The file itself is not changed but other files "
                  "probably already are. Restore your game files from the "
                  "backup file %s and please report this error.",
                  file->nameUppercase, checksum, file->md5Patched,
                  bkpFilenameUppercase);
        goto fail_checksum;
#endif
    }

    if (!WriteFile(file->handle, file->data, file->size, NULL, 0)) {
        showError("Couldn't write the patched data back into file %s and "
                  "it's now probably corrupted. Replace this file with "
                  "the original in the backup file %s and try running "
                  "the patcher again.",
                  file->nameUppercase, bkpFilenameUppercase);
        goto fail_write;
    }

    return 1;

fail_write:
fail_checksum:
    return 0;
}

static int createBackup(void)
{
    HANDLE handle;
    BOOL handleOpened;
    void *data;
    char checksum[33];
    DWORD sizeInBytesLoWord;
    DWORD sizeInBytesHiWord;
    size_t sizeInBytes;
    DWORD bytesRead;
    struct PatchedFile *file;
    mz_zip_archive arch;
    BOOL writingBackup;
    BOOL readingBackup;
    void *fileData;
    size_t fileSizeInBytes;

    handleOpened = FALSE;
    data = NULL;
    fileData = NULL;
    writingBackup = FALSE;
    readingBackup = FALSE;

    handle = CreateFile(bkpFilename,
                        GENERIC_READ | GENERIC_WRITE, 0, NULL,
                        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (handle == INVALID_HANDLE_VALUE) {
        showError("Couldn't open or create the backup file "
                  "%s. If it exists, make sure it's not being used by "
                  "another application before running the patcher again.",
                  bkpFilenameUppercase);
        goto fail_open;
    }

    handleOpened = FALSE;

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        sizeInBytesLoWord = GetFileSize(handle, &sizeInBytesHiWord);

        if (sizeInBytesLoWord == INVALID_FILE_SIZE) {
            showError("The backup file %s already exists but "
                      "couldn't determine it's size. "
                      TEXT_RUN_AGAIN, bkpFilenameUppercase);
            goto fail_size;
        }
        else if (sizeInBytesHiWord || sizeInBytesLoWord > SIZE_MAX) {
            showError("The backup file %s already exists but it's too large "
                      "so its contents cannot be verified. It's unlikely "
                      "that this file was created by the patcher. "
                      TEXT_DELETE_AND_RUN_AGAIN, bkpFilenameUppercase);
        }

        sizeInBytes = sizeInBytesLoWord;

        if (!(data = malloc(sizeInBytes))) {
            showError("Couldn't allocate %lu bytes of memory. "
                      TEXT_CLOSE_PROGS_AND_RUN_AGAIN,
                      (unsigned long) sizeInBytes);
            goto fail_alloc;
        }

        if (!ReadFile(handle, data, sizeInBytes, &bytesRead,
                      NULL)) {
            showError("The backup file %s already exists but "
                      "couldn't read its contents. "
                      TEXT_CLOSE_PROGS_AND_RUN_AGAIN,
                      bkpFilenameUppercase);
            goto fail_read_existing;
        }


        memset(&arch, 0, sizeof(mz_zip_archive));

        if (mz_zip_reader_init_mem(&arch, data, sizeInBytes, 0) == MZ_FALSE) {
            showError("The backup file %s already exists but "
                      "couldn't read its contents. "
                      TEXT_CLOSE_PROGS_AND_RUN_AGAIN,
                      bkpFilenameUppercase);
            goto fail_zip_reader;
        }

        readingBackup = TRUE;

        for (file = patchedFiles; file->name; file++) {
            fileData = mz_zip_reader_extract_file_to_heap(&arch, file->name,
                                                          &fileSizeInBytes, 0);
            if (!fileData) {
                if(arch.m_last_error == MZ_ZIP_ALLOC_FAILED) {
                    showError("Couldn't allocate memory. "
                              TEXT_CLOSE_PROGS_AND_RUN_AGAIN);
                }
                else if (arch.m_last_error == MZ_ZIP_FILE_NOT_FOUND) {
                    showError("File %s was not found in the already "
                              "existing backup file %s. The backup file "
                              "might be from an earlier version of the patch "
                              "or maybe it was copied from some place else "
                              "or it was created by another program. "
                              TEXT_DELETE_AND_RUN_AGAIN,
                              file->nameUppercase,
                              bkpFilenameUppercase);
                }
                else {
                    showError("An unspecified error occured while validating "
                              "file %s in the already existing backup file "
                              "%s."
                              TEXT_RUN_AGAIN
                              "If the problem persists, restore the original "
                              "game files and delete the backup file. "
                              "A new backup file will be created by the "
                              "patcher before it modifies any files.",
                              file->nameUppercase,
                              bkpFilenameUppercase);
                }
                goto fail_zip_extract;

            }

            if (fileSizeInBytes != file->size) {
                showError("File %s in the already "
                          "existing backup file %s has an unexpected "
                          "size of %zu bytes but it should be %zu bytes."
                          "This could be caused "
                          "by an internal patcher error or maybe the "
                          "backup file was copied to the game's directory "
                          "from some place else or it was created by "
                          "another program. "
                          TEXT_DELETE_AND_RUN_AGAIN,
                          file->nameUppercase,
                          bkpFilenameUppercase,
                          fileSizeInBytes,
                          file->size);
                goto fail_size_check;

            }

            getMd5Checksum(fileData, fileSizeInBytes, checksum);

            if (strcmp(checksum, file->md5Unpatched)) {
                showError("The MD5 checksum of file %s in the already "
                          "existing backup file %s is unexpectedly\n\n"
                          "%s\n\nbut it should be\n\n%s\n\nThis could "
                          "be caused by an internal patcher error or "
                          "maybe the backup file was copied to the "
                          "game's directory from some place else "
                          "or it was created by another program. "
                          TEXT_DELETE_AND_RUN_AGAIN,
                          file->nameUppercase,
                          bkpFilenameUppercase,
                          checksum,
                          file->md5Unpatched);
                goto fail_checksum_check;
            }

            free(fileData);
            fileData = NULL;
        }

        readingBackup = FALSE;

        if (mz_zip_reader_end(&arch) == MZ_FALSE) {
            goto fail_zip_end_read;
        }

        free(data);
        data = NULL;
    }
    else {
        for (file = patchedFiles; file->name; file++) {
            if (file->alreadyPatched) {
                showError("Couldn't write the file %s to the backup file %s "
                          "because it's already patched. The backup file is "
                          "now in an inconsistent state. In order to run the "
                          "patcher successfully you need to somehow restore "
                          "the original file %s and delete the corrupted "
                          "backup file %s.",
                          file->nameUppercase,
                          bkpFilenameUppercase,
                          file->nameUppercase,
                          bkpFilenameUppercase);
                goto fail_already_patched;
            }
        }

        memset(&arch, 0, sizeof(mz_zip_archive));

        if (mz_zip_writer_init_heap(&arch, 0, sizeInBytes) == MZ_FALSE) {
            showError(TEXT_INCONSISTENT_BACKUP, bkpFilenameUppercase);
            goto fail_zip_writer;
        }

        writingBackup = TRUE;

        for (file = patchedFiles; file->name; file++) {
            if (mz_zip_writer_add_mem(&arch, file->nameInFilesystemCase,
                                      file->data, file->size,
                                      MZ_DEFAULT_COMPRESSION)  == MZ_FALSE) {
                showError(TEXT_INCONSISTENT_BACKUP, bkpFilenameUppercase);
                goto fail_zip_add;
            }
        }

        if (mz_zip_writer_finalize_heap_archive(&arch, &data,
                                                &sizeInBytes) == MZ_FALSE) {
            showError(TEXT_INCONSISTENT_BACKUP, bkpFilenameUppercase);
            goto fail_zip_finalize;
        }

        if (mz_zip_writer_end(&arch) == MZ_FALSE) {
            showError(TEXT_INCONSISTENT_BACKUP, bkpFilenameUppercase);
            goto fail_zip_end_write;

        }

        writingBackup = FALSE;

        if (!WriteFile(handle, data, sizeInBytes, NULL, 0)) {
            showError(TEXT_INCONSISTENT_BACKUP, bkpFilenameUppercase);
            goto fail_write;
        }

        free(data);
        data = NULL;
    }

    bkpHandle = handle;

    return 1;

fail_write:
fail_zip_end_write:
fail_zip_finalize:
fail_zip_add:
    if (writingBackup) {
        mz_zip_writer_end(&arch);
    }
fail_zip_writer:
fail_already_patched:
fail_zip_end_read:
fail_checksum_check:
fail_size_check:
fail_zip_extract:
    if (readingBackup) {
        mz_zip_reader_end(&arch);
    }
fail_zip_reader:
fail_read_existing:
    if (data) {
        free(data);
    }
fail_alloc:
fail_size:
    if (handleOpened) {
        CloseHandle(handle);
    }
fail_open:

    return 0;
}

int cmpChanges(const void *_chA, const void *_chB)
{
    const struct PatchedBytes *chA;
    const struct PatchedBytes *chB;
    int res;

    chA = *((struct PatchedBytes **) _chA);
    chB = *((struct PatchedBytes **) _chB);

    if (!chA->fnamePrefix) {
        res = 1;
    }
    else {
        if (!chB->fnamePrefix) {
            res = -1;
        }
        else {
            res = _stricmp(chA->fnamePrefix, chB->fnamePrefix);
            if (!res) {
                if (chA->addr == chB->addr) {
                    res = 0;
                }
                else {
                    res = chA->addr < chB->addr ? -1 : 1;
                }
            }
        }
    }

    return res;
}

static int patchData()
{
    struct PatchedFile *file;
    const struct PatchedBytes **sortedChanges;
    const struct Section *sec;
    size_t numChanges;
    const char *fnamePrefix;
    const struct PatchedBytes **ch;
    DWORD prevChangeAddr;
    DWORD addrFirst;
    DWORD addrLast;
    const char *bytes;
    size_t bytesLen;
    int hexValA;
    int hexValB;
    DWORD firstByteIdx;
    DWORD nextByteIdx;
    size_t ii;

    fnamePrefix = NULL;
    prevChangeAddr = 0;

    assert(!fnamePrefix);
    assert(!prevChangeAddr);

    /* FIXME: Add checks for undefined behavior. */

    /* Sort the changes on prefix and size. */

    numChanges = 0;
    while (changes[numChanges].fnamePrefix) {
        numChanges++;
    }

    /* Increment a final time to account for the end-of-array marker. */

    numChanges++;

    sortedChanges = _alloca(numChanges * sizeof(struct PatchedBytes *));

    for (ii = 0; ii < numChanges; ii++) {
        sortedChanges[ii] = &changes[ii];
    }

    qsort(sortedChanges, numChanges, sizeof(struct PatchedBytes *), cmpChanges);

    for (ch = sortedChanges; (*ch)->fnamePrefix; ch++) {
        if (!fnamePrefix || _stricmp(fnamePrefix, (*ch)->fnamePrefix)) {
            for (file = patchedFiles; file->name; file++) {
                if (!_stricmp(file->namePrefix, (*ch)->fnamePrefix)) {
                    break;
                }
            }

            if (!file->name) {
                showError("Internal patcher error, failed to find the file to "
                          "patch for the change set with prefix %s and "
                          "address 0x%08lX. "
                          TEXT_PLEASE_REPORT,
                          (*ch)->fnamePrefix,
                          (unsigned long) (*ch)->addr);
                goto fail_unknown_file;
            }

            fnamePrefix = (*ch)->fnamePrefix;
            nextByteIdx = 0;
        }

        for (sec = file->sections; sec->name; sec++) {
            addrFirst = file->imageBase + sec->virtualAddr;
            addrLast = addrFirst + sec->size;

            if ((*ch)->addr >= addrFirst && (*ch)->addr <= addrLast) {
                break;
            }
        }

        if (!sec->name) {
            showError("Internal patcher error, failed to find the correct "
                      "section of file %s for the change set with prefix %s "
                      "and address 0x%08lX. "
                      TEXT_PLEASE_REPORT,
                      file->name,
                      (*ch)->fnamePrefix,
                      (unsigned long) (*ch)->addr);
            goto fail_unknown_sec;
        }

        firstByteIdx = sec->rawDataPtr
                       + ((*ch)->addr - sec->virtualAddr - file->imageBase);

        if (firstByteIdx < nextByteIdx) {
            showError("Internal patcher error, at least two change sets for "
                      "the file %s (change prefix %s) overlap. "
                      TEXT_PLEASE_REPORT,
                      file->nameUppercase,
                      fnamePrefix);
            goto fail_overlapping_secs;
        }

        /* Apply the changes to the data. */

        nextByteIdx = firstByteIdx;
        bytes = (*ch)->bytes;
        bytesLen = strlen(bytes);

        ii = 0;
        while (ii < bytesLen) {
            if (!isspace(bytes[ii])) {
                if (ii == bytesLen - 1
                    || (hexValA = hexDigitToNum(bytes[ii])) == -1
                    || (hexValB = hexDigitToNum(bytes[ii + 1])) == -1) {
                    showError("Internal patcher error, invalid byte "
                              "representation for the change set with "
                              "prefix %s and address 0x%08lX. "
                              TEXT_PLEASE_REPORT,
                              (*ch)->fnamePrefix,
                              (unsigned long) (*ch)->addr);
                    goto fail_invalid_hex_str;
                }

                if (nextByteIdx == file->size) {
                    showError("Internal patcher error, change set with prefix "
                              "%s and address extends past the end of the "
                              "file %s. ",
                              TEXT_PLEASE_REPORT,
                              (*ch)->fnamePrefix,
                              (unsigned long) (*ch)->addr,
                              file->nameUppercase);
                    goto fail_invalid_byte_addr;
                }

                ((unsigned char *) file->data)[nextByteIdx++] = hexValA * 16
                                                                + hexValB;
                ii += 2;
            }
            else {
                ii++;
            }
        }
    }

    return 1;

fail_invalid_byte_addr:
fail_invalid_hex_str:
fail_overlapping_secs:
fail_unknown_sec:
fail_unknown_file:
    return 0;
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prevInst, LPSTR cmdLine,
                   int nShowCmd)
{
    struct PatchedFile *file;

    for (file = patchedFiles; file->name; file++) {
        if (!readFile(file)) {
            goto fail_read_file;
        }
    }

    if (allFilesPatched) {
        showInfo("Nothing will be done, the game files are already patched.");
        goto all_patched;
    }

    if (!createBackup()) {
        goto fail_backup;
    }

    if (!patchData()) {
        goto fail_patch;
    }

    for (file = patchedFiles; file->name; file++) {
        if (!file->alreadyPatched && !writeFile(file)) {
            goto fail_write_file;
        }
    }

    closeFiles();
    CloseHandle(bkpHandle);
    showInfo("Patching was successful. A number of games files were modified "
             "and a backup file %s containing the originals was created in "
             "the same directory. If you wish to uninstall the patch just "
             "the contents of the backup in the game directory. Enjoy "
             "Hitman: Codename 47!", bkpFilenameUppercase);

all_patched:
    closeFiles();
    return 0;

fail_write_file:
fail_patch:
fail_backup:
fail_read_file:
    closeFiles();
    return 1;
}

