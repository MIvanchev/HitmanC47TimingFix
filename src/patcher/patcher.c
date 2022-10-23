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
#include "md5.h"

#define BUFLEN(b) (sizeof((b)) / sizeof((b)[0]))

struct section {
    const char *name;
    DWORD virtualAddr;
    DWORD size;
    DWORD rawDataPtr;
};

struct patched_bytes {
    DWORD addr;
    const char *bytes;
};

static const char *filename = "SYSTEM.DLL";
static const char *backupFilename = "SYSTEM.DLL.BAK";
static DWORD fileSizeInBytes = 278528;
static DWORD imageBase = 0x0FFA0000;
static char *oldMd5 = "90b1ac786841bdd5f45077b97fdb83ee";
static char *newMd5 = "a59bc56db5feeb7c8537d1608bbb942d";

static struct section sections[] = {
    { ".text", 0x1000, 0x32000, 0x1000 },
    { ".rdata", 0x33000, 0x5000, 0x33000 },
    { ".data", 0x38000, 0x8000, 0x38000 },
    { NULL, 0 }
};

/* *INDENT-OFF* */
static struct patched_bytes changes[] = {
    { 0x0FFAE159, "8d 5d e8 53 ff 15 ec ae fd 0f 90 90 90 90 90" },
    { 0x0FFAE222, "8d 5d f4 53 ff 15 ec ae fd 0f 90 90 90 90 90" },
    { 0x0FFB1080, "e8 63 02 00 00 90 90 90 90 90" },
    { 0x0FFB1230, "dd 81 61 0a 00 00 dc 1d 30 37 fd 0f df e0 f6 c4"
                  "40 75 01 c3 50 50 53 89 cb 56 ba b8 ae fd 0f 52"
                  "b2 ec 52 b2 d4 52 b2 a8 52 ff 15 78 30 fd 0f 89"
                  "c6 50 ff 15 5c 30 fd 0f 5a 89 02 56 ff 15 5c 30"
                  "fd 0f 8d 74 24 08 56 ff d0 df 2e dd 1e 5e a1 24"
                  "30 fd 0f 8b 08 8a 81 f1 38 00 00 84 c0 75 2f 8b"
                  "15 28 30 fd 0f 8b 0a 8b 01 68 dc 08 00 00 68 68"
                  "a2 fd 0f ff 50 20 8b 54 24 08 8b 08 52 8b 54 24"
                  "08 52 68 cc aa fd 0f 50 ff 51 2c 83 c4 10 a1 44"
                  "9a fd 0f 85 c0 75 06 ff 15 4c 9a fd 0f 8b 44 24"
                  "04 8b 4c 24 08 89 83 61 0a 00 00 89 8b 65 0a 00"
                  "00 5b 58 58 c3 8d 76 00 75 04 dd d8 d9 e8 6a 2d"
                  "db 04 24 58 d9 e8 d8 f1 d8 da df e0 f6 c4 41 75"
                  "04 d8 c9 dc c9 dd d8 c3" },
    { 0x0FFB1738, "89 03 89 43 04 eb 1c" },
    { 0x0FFB174C, "a1 ec ae fd 0f 8d 5d ec 85 c0 74 e0 53 ff d0" },
    { 0x0FFDAEA8, "6b 65 72 6e 65 6c 33 32 2e 64 6c 6c 00 00 00 00"
                  "51 75 65 72 79 50 65 72 66 6f 72 6d 61 6e 63 65"
                  "46 72 65 71 75 65 6e 63 79 00 00 00 51 75 65 72"
                  "79 50 65 72 66 6f 72 6d 61 6e 63 65 43 6f 75 6e"
                  "74 65 72 00" },
    { 0, NULL }
};
/* *INDENT-ON* */

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
#define showError(...) showMsg(MB_OK | MB_ICONERROR, __VA_ARGS__)

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

static void getMd5Checksum(BYTE *data, DWORD size, char *checksum)
{
    MD5Context ctx;
    size_t ii;

    assert(data);
    assert(size);
    assert(checksum);

    md5Init(&ctx);
    md5Update(&ctx, data, size);
    md5Finalize(&ctx);

    for (ii = 0; ii < 16; ii++)  {
        sprintf(&checksum[2*ii], "%02x", (unsigned int) ctx.digest[ii]);
    }
}

static int patchData(BYTE *data)
{
    struct section *sec;
    struct patched_bytes *change;
    DWORD addrFirst;
    DWORD addrLast;
    const char *bytes;
    size_t bytesLen;
    int hexValA;
    int hexValB;
    DWORD firstByteIdx;
    DWORD nextByteIdx;
    size_t ii;
    int res;

    res = 1;
    nextByteIdx = 0;

    assert(data);
    assert(res);
    assert(!nextByteIdx);

    /* TODO: Add checks for undefined behavior. */

    for (change = changes; change->bytes; change++) {

        /* Find the section */

        for (sec = sections; sec->name; sec++) {
            addrFirst = imageBase + sec->virtualAddr;
            addrLast = addrFirst + sec->size;

            if (change->addr >= addrFirst && change->addr <= addrLast) {
                break;
            }
        }

        if (!sec->name) {
            showError("Internal patcher error, failed to find the correct "
                      "section for the change at address 0x%08lX.",
                      (unsigned long) change->addr);
            res = 0;
            break;
        }

        firstByteIdx = sec->rawDataPtr
                       + (change->addr - sec->virtualAddr - imageBase);

        if (firstByteIdx < nextByteIdx) {
            showError("Internal patcher error, the change sets should be "
                      "listed in ascending order and must not overlap.");
            res = 0;
            break;
        }

        /* Apply the changes to the data. */

        nextByteIdx = firstByteIdx;
        bytes = change->bytes;
        bytesLen = strlen(bytes);

        ii = 0;
        while (ii < bytesLen) {
            if (!isspace(bytes[ii])) {
                if (ii == bytesLen - 1
                    || (hexValA = hexDigitToNum(bytes[ii])) == -1
                    || (hexValB = hexDigitToNum(bytes[ii + 1])) == -1) {
                    showError("Internal patcher error, invalid byte "
                              "representation for the change at address "
                              "0x%08lX.", (unsigned long) change->addr);
                    res = 0;
                    break;
                }

                if (nextByteIdx == fileSizeInBytes) {
                    showError("Internal patcher error, change at address "
                              "0x%08lX extends past the end of the file.",
                              (unsigned long) change->addr);
                    res = 0;
                    break;
                }

                data[nextByteIdx++] = hexValA * 16 + hexValB;
                ii += 2;
            }
            else {
                ii++;
            }
        }

        if (!res) {
            break;
        }
    }

    return res;
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prevInst, LPSTR cmdLine,
                   int nShowCmd)
{

    HANDLE fileHandle;
    BYTE *fileData;
    HANDLE backupHandle;
    BYTE *backupData;
    DWORD bytesRead;
    char checksum[33];
    int res;

    assert(filename);
    assert(strlen(filename));
    assert(backupFilename);
    assert(strlen(backupFilename));
    assert(imageBase <= 0xFFFFFFFF);
    assert(strcmp(filename, backupFilename));
    assert(fileSizeInBytes);
    assert(BUFLEN(checksum) >= 32 + 1);

    fileData = NULL;
    backupData = NULL;
    res = 1;

    /* Read file to be patched and verify the checksum. */

    fileHandle = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        showError("Patching failed. Couldn't open the file %s. Make sure the "
                  "patcher is located in the game's main directory containing "
                  "HITMAN.EXE.", filename);
        goto fail_open;
    }

    if (!(fileData = malloc(fileSizeInBytes + 1))) {
        showError("Patching failed. Couldn't allocate %lu bytes of memory.",
                  (unsigned long) (fileSizeInBytes + 1));
        goto fail_alloc_file_data;
    }

    if (!ReadFile(fileHandle, fileData, fileSizeInBytes + 1, &bytesRead,
                  NULL)) {
        showError("Patching failed. Couldn't read the contents of the file %s.",
                  filename);
        goto fail_read_file;
    }

    if (bytesRead != fileSizeInBytes) {
        showError("Patching failed. The file %s has an unexpected file size of "
                  "%lu bytes, but it should be exactly %lu bytes. Maybe "
                  "another patch was applied or this is an unsupported game "
                  "version.", filename, (unsigned long) bytesRead,
                  (unsigned long) fileSizeInBytes);
        goto fail_invalid_file_size;
    }

    getMd5Checksum(fileData, fileSizeInBytes, checksum);

    if (strcmp(checksum, oldMd5)) {
        if (!strcmp(checksum, newMd5)) {
            showInfo("Patching failed. The file %s has an MD5 checksum of\n\n"
                     "%s\n\nwhich indicates that it was already patched.",
                     filename, checksum);
            goto fail_already_patched;
        }
        else {
            showError("Patching failed. The file %s has an MD5 checksum of\n\n"
                      "%s\n\nbut it should be\n\n%s\n\nMaybe another patch was "
                      "applied or this is an unsupported game version.",
                      filename, checksum, oldMd5);
            goto fail_invalid_checksum;
        }
    }

    /* Read an existing backup file and verify the contents or create a new
    ** backup file.
    */

    backupHandle = CreateFile(backupFilename, GENERIC_READ | GENERIC_WRITE, 0,
                              NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (backupHandle == INVALID_HANDLE_VALUE) {
        showError("Patching failed. Couldn't open or create the backup file "
                  "%s.", backupFilename);
        goto fail_backup_open;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (!(backupData = malloc(fileSizeInBytes + 1))) {
            showError("Patching failed. Couldn't allocate %lu bytes of memory.",
                      (unsigned long) (fileSizeInBytes + 1));
            goto fail_alloc_backup_data;
        }

        if (!ReadFile(backupHandle, backupData, fileSizeInBytes + 1, &bytesRead,
                      NULL)) {
            showError("Patching failed. The backup file %s already exists but "
                      "couldn't read its contents.", backupFilename);
            goto fail_backup_read;
        }

        if (bytesRead != fileSizeInBytes) {
            showError("Patching failed. The backup file %s has an unexpected "
                      "file size of %lu bytes, but it should be exactly %lu "
                      "bytes. Maybe it's a backup of a previously applied "
                      "patch.", backupFilename, (unsigned long) bytesRead,
                      (unsigned long) fileSizeInBytes);
            goto fail_invalid_backup_size;
        }

        if (memcmp(fileData, backupData, fileSizeInBytes * sizeof(BYTE))) {
            showError("Patching failed. The backup file %s contains data "
                      "different from the file %s.", backupFilename, filename);
            goto fail_invalid_backup_data;
        }

        free(backupData);
        backupData = NULL;

        if (SetFilePointer(backupHandle, 0, NULL,
                           FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
            showError("Patching failed. Couldn't prepare the backup file %s "
                      "for writing after having read its contents.",
                      backupFilename);
            goto fail_backup_rewind;
        }
    }

    if (!WriteFile(backupHandle, fileData, fileSizeInBytes, NULL, 0)) {
        showError("Patching failed. The backup file %s was created but "
                  "couldn't write data to it. Delete the file manually before "
                  "running the patcher again.", backupFilename);
        goto fail_backup_write;
    }

    /* Patch the data and write the patched file. */

    if (!patchData(fileData)) {
        goto fail_patch;
    }

    if (SetFilePointer(fileHandle, 0, NULL,
                       FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        showError("Patching failed. Couldn't prepare the file %s for "
                  "writing after having read its contents. A backup file %s "
                  "was created which you need to delete manually if not "
                  "needed.", filename, backupFilename);
        goto fail_backup_rewind;
    }
#if 1
    if (!WriteFile(fileHandle, fileData, fileSizeInBytes, NULL, 0)) {
        showError("Patching failed. Couldn't write the patched data back into "
                  "file %s. The file is probably corrupted. Replace this file "
                  "with the backup file %s before playing the game or "
                  "rerunning the patcher.",
                  filename, backupFilename);
        goto fail_backup_write;
    }
#endif

    free(fileData);
    CloseHandle(backupHandle);
    CloseHandle(fileHandle);

    showInfo("Patching was successful. The file %s was modified and a backup "
             "file %s was created in the same directory. If you wish to "
             "uninstall the patch just overwrite the file with the backup. "
             "Enjoy Hitman at sane speed!", filename, backupFilename);

    return 0;

fail_backup_write:
fail_backup_rewind:
fail_patch:
fail_invalid_backup_data:
fail_invalid_backup_size:
fail_backup_read:
    if (backupData != NULL) {
        free(backupData);
    }
fail_alloc_backup_data:
    CloseHandle(backupHandle);
fail_backup_open:
fail_invalid_checksum:
fail_already_patched:
fail_invalid_file_size:
fail_read_file:
    free(fileData);
fail_alloc_file_data:
    CloseHandle(fileHandle);
fail_open:
    return res;
}

