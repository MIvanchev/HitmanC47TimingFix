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

#define UNICODE
#include <windows.h>
#include <assert.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>
#include "md5.h"
#include "miniz.h"
#include "patches.h"

#define DWORD_MAX 0xFFFFFFFF

#define BUFLEN(buf) (sizeof((buf)) / sizeof((buf)[0]))
#define MIN(valA, valB) (((valA) <= (valB)) ? (valA) : (valB))
#define MAX(valA, valB) (((valA) >= (valB)) ? (valA) : (valB))

#define _static_assert(expr) _Static_assert((expr), #expr)

#define TEXT_RUN_AGAIN L"Try running the patcher again."
#define TEXT_CLOSE_PROGS_AND_RUN_AGAIN L"Close some programs and try running " \
                                       L"the patcher again."
#define TEXT_DELETE_AND_RUN_AGAIN L"In order to run the patcher successfully " \
                                  L"you must restore the original game files " \
                                  L"and delete the backup file. A new backup " \
                                  L"file will be created by the patcher " \
                                  L"before it modifies any files."
#define TEXT_INCONSISTENT_BACKUP L"An error occured while writing the " \
                                 L"backup file %s. This file now has an " \
                                 L"inconsistent state and you need " \
                                 L"to delete it before rerunning the patcher."
#define TEXT_PATCHER_BUG L"This is likely due to a bug in the patcher. " \
                         L"Please report it."
#define TEXT_PLEASE_REPORT L"Please report this error."

#define VALIDATE_CHECKSUMS

struct Section {
    const char *name;
    DWORD virtualAddr;
    DWORD size;
    DWORD rawDataPtr;
};

struct PatchedFile {
    const wchar_t *name;
    const wchar_t *nameUppercase;
    const wchar_t *namePrefix;
    const size_t size;
    const DWORD imageBase;
    const char *md5Unpatched;
    const char *md5Patched;
    const struct Section sections[4];
    wchar_t *nameInFsCase;
    bool handleOpened;
    HANDLE handle;
    bool alreadyPatched;
    bool modified;
    void *data;
};

struct PatchedFile patchedFiles[] = {
#ifdef PATCH_FOR_GOG
    {
        L"system.dll",
        L"SYSTEM.DLL",
        L"system",
        278528,
        0x0FFA0000,
        "90b1ac786841bdd5f45077b97fdb83ee",
        "a2793eb85fd4bbdfb2941586ba640f54",
        {   { ".text", 0x1000, 0x32000, 0x1000 },
            { ".rdata", 0x33000, 0x5000, 0x33000 },
            { ".data", 0x38000, 0x8000, 0x38000 },
            { NULL, 0 }
        }
    },
    {
        L"HitmanDlc.dlc",
        L"HITMANDLC.DLC",
        L"hitmandlc",
        2555904,
        0x0FCC0000,
        "bf3e32ba24d2816adb8ba708774f1e1a",
        "0e930a1a24ec7cbe3452960562aa40b9",
        {   { ".text", 0x1000, 0x1ef000, 0x1000 },
            { ".rdata", 0x1f0000, 0x2f000, 0x1f0000 },
            { ".data", 0x21f000, 0x25000, 0x21f000 },
            { NULL, 0 }
        },
    },
#elif defined PATCH_FOR_OTHER
    {
        L"system.dll",
        L"SYSTEM.DLL",
        L"system",
        278528,
        0x0FFA0000,
        "6d3bcfab731dbbbf555d054ccbad6eda",
        "91c94a3f6bfd3bf2b506b37b17a189d0",
        {   { ".text", 0x1000, 0x32000, 0x1000 },
            { ".rdata", 0x33000, 0x5000, 0x33000 },
            { ".data", 0x38000, 0x8000, 0x38000 },
            { NULL, 0 }
        }
    },
    {
        L"HitmanDlc.dlc",
        L"HITMANDLC.DLC",
        L"hitmandlc",
        2555904,
        0x0FCC0000,
        "9b32d467c3d62e9ec485de7b7586fed3",
        "a7e4ffefbb3e6c4c0461a5f05ac627e5",
        {   { ".text", 0x1000, 0x1ef000, 0x1000 },
            { ".rdata", 0x1f0000, 0x2f000, 0x1f0000 },
            { ".data", 0x21f000, 0x25000, 0x21f000 },
            { NULL, 0 }
        },
    },
#else
#error You need to specify which game distribution to build the patcher for.
#endif
    { NULL }
};

static wchar_t *pathSep;
static size_t pathSepLen;
static wchar_t *parentPath;
static size_t parentPathLen;
static const wchar_t *bkpFilename = L"patch-backup.zip";
static const wchar_t *bkpFilenameUppercase = L"PATCH-BACKUP.ZIP";
static HANDLE bkpHandle;
static bool allFilesPatched = true;

static void showMsg(int type, const wchar_t *msg, ...)
{
    va_list args;
    wchar_t buf[512];

    _static_assert(BUFLEN(buf) > 256);
    assert(msg);

    va_start(args, msg);
    _vsnwprintf(buf, BUFLEN(buf), msg, args);
    va_end(args);

    MessageBox(NULL, buf, L"Patcher", type);
}

#define showInfo(...) showMsg(MB_OK | MB_ICONINFORMATION, __VA_ARGS__)
#define showError(...) showMsg(MB_OK | MB_ICONERROR, \
                               "Patching failed. " __VA_ARGS__)

static int hexDigitToNum(char digit)
{
    int res;

    res = -1;

    assert(res == -1);

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

static int isBufAllocable(size_t factor, const size_t *size_components)
{
    size_t sum;
    size_t ii;

    sum = 0;

    assert(factor);
    assert(size_components);
    assert(size_components[0]);
    assert(!sum);

    for (ii = 0; size_components[ii] != 0; ii++) {
        if ((SIZE_MAX - sum) / factor < size_components[ii]) {
            sum = 0;
            break;
        }

        sum += size_components[ii];
    }

    return sum * factor;
}

static void *allocMem(size_t len)
{
    void *buf;

    assert(len);

    if (!(buf = malloc(len))) {
        showError(L"Couldn't allocate %zu bytes of memory. "
                  TEXT_CLOSE_PROGS_AND_RUN_AGAIN,
                  len);
    }

    return buf;
}

static void *reallocMem(void *buf, size_t len, bool reportError)
{
    assert(buf);
    assert(len);

    buf = realloc(buf, len);

    if (!buf && reportError) {
        showError(L"Couldn't allocate %zu bytes of memory. "
                  TEXT_CLOSE_PROGS_AND_RUN_AGAIN,
                  len);
    }

    return buf;
}

static void getMd5Checksum(void *data, DWORD size, char *checksum)
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

static const wchar_t *getFilenameFromPath(const wchar_t *path)
{
    const wchar_t *res;
    wchar_t buf_name[_MAX_FNAME+1];
    wchar_t buf_ext[_MAX_EXT+1];
    wchar_t buf[_MAX_FNAME+_MAX_EXT+1];
    const wchar_t *ptr;

    res = NULL;

    _static_assert(BUFLEN(buf_name) > _MAX_FNAME);
    _static_assert(BUFLEN(buf_ext) > _MAX_FNAME);
    _static_assert(BUFLEN(buf) > _MAX_FNAME+_MAX_EXT);
    assert(path);
    assert(!res);

    if (!_wsplitpath_s(path, NULL, 0, NULL, 0, buf_name, BUFLEN(buf_name), buf_ext,
                       BUFLEN(buf_ext))) {
        wcscpy(buf, buf_name);
        wcscat(buf, buf_ext);

        // Go back from the null terminator until the strings compare equal.

        ptr = &path[wcslen(path)];

        do {
            if (!wcscmp(ptr, buf)) {
                res = ptr;
                break;
            }

            if (ptr == path) {
                break;
            }

            ptr = CharPrev(path, ptr);
        }
        while (1);
    }

    return res;
}

static wchar_t *getPatcherParentPath(void)
{
    wchar_t *buf;
    size_t bufLen;
    wchar_t *ptr;
    wchar_t *res;
    const size_t limit = MIN(DWORD_MAX, SIZE_MAX) / sizeof(wchar_t);
    DWORD len;
    const wchar_t *fnamePtr;
    wchar_t *fnameSeparatorPtr;

    buf = NULL;
    bufLen = 0;

    _static_assert(MAX_PATH == 260);
    // FIXME: This is actually a static assert but mingw-w64 whines it's not
    // a constant expression...
    assert(limit == MIN(SIZE_MAX, DWORD_MAX) / sizeof(wchar_t));
    assert(!buf);
    assert(!bufLen);

    do {
        if (bufLen == limit) {
            showError(L"An unrealistic amount of memory is required to "
                      L"determine the absolute path to the patcher. "
                      TEXT_PATCHER_BUG);
            goto fail_limit;
        }
        else {
            bufLen += MIN(MAX_PATH, limit - MAX_PATH);
        }

        if (!buf) {
            if (!(buf = allocMem(bufLen * sizeof(wchar_t)))) {
                goto fail_alloc;
            }
        }
        else {
            if (!(ptr = reallocMem(buf, bufLen * sizeof(wchar_t), 1))) {
                goto fail_realloc;
            }

            buf = ptr;
        }

        SetLastError(ERROR_SUCCESS);

        len = GetModuleFileName(NULL, buf, bufLen);
        if (!len) {
            showError(L"Couldn't determine the absolute path to the patcher. "
                      TEXT_RUN_AGAIN);
            goto fail_fname;
        }

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            break;
        }
    }
    while (1);

    if (!(fnamePtr = getFilenameFromPath(buf))
        || fnamePtr == buf
        || wcsncmp(pathSep, (fnameSeparatorPtr = CharPrev(buf, fnamePtr)),
                   pathSepLen)) {
        showError(L"Couldn't determine the directory in the absolute path "
                  L"to the patcher %s. "
                  TEXT_PATCHER_BUG,
                  buf);
        goto fail_path;
    }

    // We are effectively shortening buf so no need to check for limits.

    *fnameSeparatorPtr = L'\0';

    if (!(res = reallocMem(buf, (wcslen(buf) + 1) * sizeof(wchar_t), 0))) {
        res = buf;
    }

    return res;

fail_path:
fail_limit:
fail_fname:
fail_realloc:
    free(buf);
fail_alloc:
    return NULL;
}

static wchar_t *getAbsolutePathInFsCase(const wchar_t *name)
{
    wchar_t *buf;
    size_t bufLen;
    HANDLE handle;
    WIN32_FIND_DATA findData;
    wchar_t *res;
    size_t resLen;

    assert(name);

    bufLen = isBufAllocable(sizeof(wchar_t), (size_t[]) {
        parentPathLen, pathSepLen, wcslen(name), 1, 0
    });

    if (!bufLen) {
        showError(L"An unrealistic amount of memory is required to extract the "
                  L"absolute path to file %s. " TEXT_PATCHER_BUG,
                  name);
        goto fail_lookup_path_too_long;
    }

    if (!(buf = allocMem(bufLen))) {
        goto fail_alloc;
    }

    wcscpy(buf, parentPath);
    wcscat(buf, pathSep);
    wcscat(buf, name);

    handle = FindFirstFile(buf, &findData);
    if (handle == INVALID_HANDLE_VALUE) {
        showError(L"Couldn't locate file %s in the directory of the "
                  L"patcher %s. Make sure the patcher is located in "
                  L"the game's main directory containing HITMAN.EXE "
                  L"and run the patcher again.",
                  name,
                  parentPath);
        goto fail_find;
    }

    FindClose(handle);

    if (_wcsicmp(name, findData.cFileName)) {
        showError(L"Couldn't query the file system name of file %s "
                  L"in the directory of the patcher %s. The query "
                  L"returned the result %s. "
                  TEXT_PATCHER_BUG, name, parentPath, findData.cFileName);
        goto fail_invalid_name;
    }

    // findData.cFileName guaranteed to be at most MAX_PATH.

    resLen = isBufAllocable(sizeof(wchar_t), (size_t[]) {
        parentPathLen, pathSepLen, wcslen(findData.cFileName), 1, 0
    });

    if (!resLen) {
        showError(L"An unrealistic amount of memory is required to store the "
                  L"absolute path to file %s. " TEXT_PATCHER_BUG,
                  name);
        goto fail_real_path_too_long;
    }

    if (resLen != bufLen) {
        if (!(res = reallocMem(buf, resLen, resLen > bufLen))) {
            if (resLen > bufLen) {
                goto fail_expand;
            }

            res = buf;
        }
    }
    else {
        res = buf;
    }

    wcscpy(res, parentPath);
    wcscat(res, pathSep);
    wcscat(res, findData.cFileName);

    return res;

fail_expand:
fail_real_path_too_long:
fail_invalid_name:
fail_find:
    free(buf);
fail_alloc:
fail_lookup_path_too_long:
    return NULL;
}

static void closeFiles(void)
{
    struct PatchedFile *file;

    for (file = patchedFiles; file->name; file++) {
        if (file->handleOpened) {
            CloseHandle(file->handle);
            file->handleOpened = false;
        }
        if (file->data) {
            free(file->data);
            file->data = NULL;
        }
        if (file->nameInFsCase) {
            free(file->nameInFsCase);
            file->nameInFsCase = NULL;
        }
    }
}

static bool readFile(struct PatchedFile *file)
{
    wchar_t *absPath;
    wchar_t *nameInFsCase;
    const wchar_t *ptr;
    HANDLE handle;
    size_t sizeInBytes;
    void *data;
    DWORD bytesRead;
    char checksum[33];
    bool alreadyPatched;

    absPath = NULL;
    nameInFsCase = NULL;
    data = NULL;
    alreadyPatched = false;

    assert(file);
    assert(file->size < MIN(SIZE_MAX, DWORD_MAX));
    assert(!absPath);
    assert(!nameInFsCase);
    assert(!data);
    assert(!alreadyPatched);

    if (!(absPath = getAbsolutePathInFsCase(file->nameUppercase))) {
        goto fail_abs_path;
    }

    // Read file to be patched and verify the checksum.

    handle = CreateFile(absPath, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        showError(L"Couldn't open the file %s. Make sure the "
                  L"file is not being used by another application and the "
                  L"patcher is located in the game's main directory containing "
                  L"HITMAN.EXE.",
                  file->nameUppercase);
        goto fail_open;
    }

    if (!(ptr = getFilenameFromPath(absPath))) {
        showError(L"Couldn't extract the file name from the absolute path "
                  L"%s. " TEXT_PATCHER_BUG,
                  absPath);
        goto fail_fs_case;
    }

    // Use a temp buffer avoid memory overlap in wcscpy which is undefined.

    nameInFsCase = _alloca((wcslen(ptr) + 1) * sizeof(wchar_t));
    wcscpy(nameInFsCase, ptr);
    wcscpy(absPath, nameInFsCase);

    if (!(nameInFsCase = reallocMem(absPath,
                                    (wcslen(absPath) + 1) * sizeof(wchar_t), 0))) {
        nameInFsCase = absPath;
    }

    absPath = NULL;

    // Allocate an additional byte of space so we can test for the expected
    // file size.

    sizeInBytes = file->size + 1;
    if (!(data = allocMem(sizeInBytes))) {
        goto fail_alloc;
    }

    if (!ReadFile(handle, data, file->size + 1, &bytesRead, NULL)) {
        showError(L"Couldn't read the contents of the file %s.",
                  file->nameUppercase);
        goto fail_read;
    }

    if (bytesRead != file->size) {
        showError(L"The file %s has an unexpected file size of "
                  L"%lu bytes, but it should be exactly %lu bytes. This could "
                  L"be caused by a failed prior patching attempt, the "
                  L"application of another patch or an supported game version.",
                  file->nameUppercase,
                  (unsigned long) bytesRead,
                  (unsigned long) file->size);
        goto fail_invalid_size;
    }

    getMd5Checksum(data, file->size, checksum);

    if (strcmp(checksum, file->md5Unpatched)) {
        if (strcmp(checksum, file->md5Patched)) {
#ifdef VALIDATE_CHECKSUMS
            showError(L"The MD5 checksum of file %s is\n\n"
                      L"%S\n\nbut it should be\n\n%S\n\nThis could be caused "
                      L"by a failed prior patching attempt, the application "
                      L"of another patch or an unsupported game version.",
                      file->nameUppercase,
                      checksum,
                      file->md5Unpatched);
            goto fail_invalid_checksum;
#endif
        }
        else {
            // Not necessarily an error yet, remember the state.
            alreadyPatched = true;
        }
    }

    if (SetFilePointer(handle, 0, NULL,
                       FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        showError(L"Couldn't prepare the file %s for writing after "
                  L"having read and verified its contents. "
                  TEXT_RUN_AGAIN, file->nameUppercase);
        goto fail_rewind;
    }

    file->data = data;
    file->nameInFsCase = nameInFsCase;
    file->handle = handle;
    file->handleOpened = false;
    file->alreadyPatched = alreadyPatched;
    file->modified = true;

    allFilesPatched = file->alreadyPatched ? allFilesPatched : false;

    return true;

fail_rewind:
fail_invalid_checksum:
fail_invalid_size:
fail_read:
    CloseHandle(handle);
fail_open:
    if (data) {
        free(data);
    }
fail_alloc:
    free(nameInFsCase);
fail_fs_case:
    if (absPath) {
        free(absPath);
    }
fail_abs_path:
    return false;
}

static bool writeFile(const struct PatchedFile *file)
{
    char checksum[33];

    _static_assert(BUFLEN(checksum) == 33);
    assert(file);

    getMd5Checksum(file->data, file->size, checksum);

    if (strcmp(checksum, file->md5Patched)) {
#ifdef VALIDATE_CHECKSUMS
        showError(L"The patched MD5 checksum of file %s is\n\n%S\n\nbut should "
                  L"be\n\n%S\n\nThis is due very likely an internal patcher "
                  L"error. The file itself is not changed but other files "
                  L"probably already are. Restore your game files from the "
                  L"backup file %s and please report this error.",
                  file->nameUppercase, checksum, file->md5Patched,
                  bkpFilenameUppercase);
        goto fail_checksum;
#endif
    }

    if (!WriteFile(file->handle, file->data, file->size, NULL, 0)) {
        showError(L"Couldn't write the patched data back into file %s and "
                  L"it's now probably corrupted. Replace this file with "
                  L"the original in the backup file %s and try running "
                  L"the patcher again.",
                  file->nameUppercase, bkpFilenameUppercase);
        goto fail_write;
    }

    return true;

fail_write:
fail_checksum:
    return false;
}

static bool readFileFromBackup(mz_zip_archive *arch,
                               const struct PatchedFile *file)
{
    char *nameUtf8;
    DWORD nameUtf8Len;
    void *data;
    size_t sizeInBytes;
    char checksum[33];

    _static_assert(BUFLEN(checksum) == 33);
    assert(arch);
    assert(file);

    // Might not be required, not sure if ZIP uses case-sensitive filenames.

    nameUtf8Len = WideCharToMultiByte(CP_UTF8, WC_NO_BEST_FIT_CHARS,
                                      file->nameInFsCase, -1, NULL, 0, NULL,
                                      NULL);
    if (!nameUtf8Len || nameUtf8Len > SIZE_MAX) {
        showError("Couldn't determine the required number of code points "
                  "to convert the file name %s from UTF16 to UTF8. "
                  TEXT_RUN_AGAIN,
                  file->nameUppercase);
        goto fail_utf8_name_len;
    }

    nameUtf8 = _alloca(nameUtf8Len);
    nameUtf8Len = WideCharToMultiByte(CP_UTF8, WC_NO_BEST_FIT_CHARS,
                                      file->nameInFsCase, -1, nameUtf8,
                                      nameUtf8Len, NULL, NULL);
    if (!nameUtf8Len) {
        showError("Couldn't convert the file name %s from UTF16 to UTF8. "
                  TEXT_RUN_AGAIN,
                  file->nameUppercase);
        goto fail_utf8_name;
    }

    data = mz_zip_reader_extract_file_to_heap(arch,
                                              nameUtf8,
                                              &sizeInBytes,
                                              MZ_ZIP_FLAG_CASE_SENSITIVE);
    if (!data) {
        // Try again disregard for case.
        data = mz_zip_reader_extract_file_to_heap(arch, nameUtf8,
                                                  &sizeInBytes, 0);
    }

    if (!data) {
        if(arch->m_last_error == MZ_ZIP_ALLOC_FAILED) {
            showError("Couldn't allocate memory. "
                      TEXT_CLOSE_PROGS_AND_RUN_AGAIN);
        }
        else if (arch->m_last_error == MZ_ZIP_FILE_NOT_FOUND) {
            showError(L"File %s was not found in the already "
                      L"existing backup file %s. The backup file "
                      L"might be from an earlier version of the patch "
                      L"or maybe it was copied from some place else "
                      L"or it was created by another program. "
                      TEXT_DELETE_AND_RUN_AGAIN,
                      file->nameUppercase,
                      bkpFilenameUppercase);
        }
        else {
            showError(L"An unspecified error occured while validating "
                      L"file %s in the already existing backup file "
                      L"%s. "
                      TEXT_RUN_AGAIN
                      L" If the problem persists, restore the original "
                      L"game files and delete the backup file. "
                      L"A new backup file will be created by the "
                      L"patcher before it modifies any files.",
                      file->nameUppercase,
                      bkpFilenameUppercase);
        }
        goto fail_extract;
    }

    if (sizeInBytes != file->size) {
        showError(L"File %s in the already "
                  L"existing backup file %s has an unexpected "
                  L"size of %zu bytes but it should be %zu bytes."
                  L"This could be caused "
                  L"by an internal patcher error or maybe the "
                  L"backup file was copied to the game's directory "
                  L"from some place else or it was created by "
                  L"another program. "
                  TEXT_DELETE_AND_RUN_AGAIN,
                  file->nameUppercase,
                  bkpFilenameUppercase,
                  sizeInBytes,
                  file->size);
        goto fail_size_check;

    }

    getMd5Checksum(data, sizeInBytes, checksum);

    free(data);
    data = NULL;

    if (strcmp(checksum, file->md5Unpatched)) {
        showError(L"The MD5 checksum of file %s in the already "
                  L"existing backup file %s is unexpectedly\n\n"
                  L"%S\n\nbut it should be\n\n%S\n\nThis could "
                  L"be caused by an internal patcher error or "
                  L"maybe the backup file was copied to the "
                  L"game's directory from some place else "
                  L"or it was created by another program. "
                  TEXT_DELETE_AND_RUN_AGAIN,
                  file->nameUppercase,
                  bkpFilenameUppercase,
                  checksum,
                  file->md5Unpatched);
        goto fail_checksum_check;
    }

    return true;

fail_checksum_check:
fail_size_check:
    if (data) {
        free(data);
    }
fail_extract:
fail_utf8_name:
fail_utf8_name_len:
    return false;
}

static bool writeFileToBackup(mz_zip_archive *arch,
                              const struct PatchedFile *file)
{
    char *nameUtf8;
    DWORD nameUtf8Len;

    assert(arch);
    assert(file);

    // Might not be required, not sure if ZIP uses case-sensitive filenames.

    nameUtf8Len = WideCharToMultiByte(CP_UTF8, WC_NO_BEST_FIT_CHARS,
                                      file->nameInFsCase, -1, NULL, 0, NULL, NULL);
    if (!nameUtf8Len || nameUtf8Len > SIZE_MAX) {
        showError(L"Couldn't determine the required number of code points "
                  L"to convert the file name %s from UTF16 to UTF8. "
                  TEXT_RUN_AGAIN,
                  file->nameUppercase);
        goto fail_utf8_name_len;
    }

    nameUtf8 = _alloca(nameUtf8Len);
    nameUtf8Len = WideCharToMultiByte(CP_UTF8, WC_NO_BEST_FIT_CHARS,
                                      file->nameInFsCase, -1, nameUtf8, nameUtf8Len, NULL, NULL);
    if (!nameUtf8Len) {
        showError(L"Couldn't convert the file name %s from UTF16 to UTF8. "
                  TEXT_RUN_AGAIN,
                  file->nameUppercase);
        goto fail_utf8_name;
    }

    if (mz_zip_writer_add_mem(arch, nameUtf8,
                              file->data, file->size,
                              MZ_DEFAULT_COMPRESSION) == MZ_FALSE) {
        showError(TEXT_INCONSISTENT_BACKUP, bkpFilenameUppercase);
        goto fail_zip_add;
    }

    return true;

fail_utf8_name_len:
fail_utf8_name:
fail_zip_add:
    return false;
}

static bool createBackup(void)
{
    size_t bkpFilenameLen;
    size_t bufLen;
    void *buf;
    HANDLE handle;
    bool handleOpened;
    void *data;
    DWORD sizeInBytesLoWord;
    DWORD sizeInBytesHiWord;
    size_t sizeInBytes;
    DWORD bytesRead;
    struct PatchedFile *file;
    mz_zip_archive arch;
    bool writingBackup;
    bool readingBackup;

    handleOpened = false;
    data = NULL;
    writingBackup = false;
    readingBackup = false;

    assert(!handleOpened);
    assert(!data);
    assert(!writingBackup);
    assert(!readingBackup);

    bkpFilenameLen = wcslen(bkpFilename);

    bufLen = isBufAllocable(sizeof(wchar_t), (size_t[]) {
        parentPathLen, pathSepLen, bkpFilenameLen, 1, 0
    });

    if (!bufLen) {
        showError(L"An unrealistic amount of memory is required to extract the "
                  L"absolute path to the backup file %s. " TEXT_PATCHER_BUG,
                  bkpFilenameUppercase);
        goto fail_path_too_long;
    }

    if (!(buf = allocMem(bufLen))) {
        goto fail_buf_alloc;
    }

    wcscpy(buf, parentPath);
    wcscat(buf, pathSep);
    wcscat(buf, bkpFilename);

    SetLastError(ERROR_SUCCESS);
   
    handle = CreateFile(buf,
                        GENERIC_READ | GENERIC_WRITE, 0, NULL,
                        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    free(buf);

    if (handle == INVALID_HANDLE_VALUE) {
        showError(L"Couldn't open or create the backup file %s in directory "
                  L"%s. If it exists, make sure it's not being used by "
                  L"another application before running the patcher again.",
                  bkpFilenameUppercase,
                  parentPath);
        goto fail_open;
    }

    handleOpened = true;

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        sizeInBytesLoWord = GetFileSize(handle, &sizeInBytesHiWord);

        if (sizeInBytesLoWord == INVALID_FILE_SIZE) {
            showError(L"The backup file %s already exists but couldn't "
                      L"determine its size. " TEXT_RUN_AGAIN,
                      bkpFilenameUppercase);
            goto fail_size;
        }
        else if (sizeInBytesHiWord || sizeInBytesLoWord > SIZE_MAX) {
            showError(L"The backup file %s already exists but it's too large "
                      L"so its contents cannot be verified. It's unlikely "
                      L"that this file was created by the patcher. "
                      TEXT_DELETE_AND_RUN_AGAIN, bkpFilenameUppercase);
        }

        sizeInBytes = sizeInBytesLoWord;

        if (!(data = allocMem(sizeInBytes))) {
            goto fail_alloc;
        }

        if (!ReadFile(handle, data, sizeInBytes, &bytesRead,
                      NULL)) {
            showError(L"The backup file %s already exists but "
                      L"couldn't read its contents. "
                      TEXT_CLOSE_PROGS_AND_RUN_AGAIN,
                      bkpFilenameUppercase);
            goto fail_read_existing;
        }

        memset(&arch, 0, sizeof(mz_zip_archive));

        if (mz_zip_reader_init_mem(&arch, data, sizeInBytes, 0) == MZ_FALSE) {
            showError(L"The backup file %s already exists but "
                      L"couldn't read its contents. "
                      TEXT_CLOSE_PROGS_AND_RUN_AGAIN,
                      bkpFilenameUppercase);
            goto fail_zip_reader;
        }

        readingBackup = true;

        for (file = patchedFiles; file->name; file++) {
            if (!readFileFromBackup(&arch, file)) {
                goto fail_read_file;
            }
        }

        readingBackup = false;

        if (mz_zip_reader_end(&arch) == MZ_FALSE) {
            goto fail_zip_end_read;
        }
    }
    else {
        for (file = patchedFiles; file->name; file++) {
            if (file->alreadyPatched) {
                showError(L"Couldn't write the file %s to the newly created "
                          L"backup file %s because it's already patched. The "
                          L"backup file will be left empty. In order to run "
                          L"the patcher successfully you need to restore the "
                          "original file %s and delete the empty backup file "
                          "%s.",
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

        writingBackup = true;

        for (file = patchedFiles; file->name; file++) {
            if (!writeFileToBackup(&arch, file)) {
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

        writingBackup = false;

        if (!WriteFile(handle, data, sizeInBytes, NULL, 0)) {
            showError(TEXT_INCONSISTENT_BACKUP, bkpFilenameUppercase);
            goto fail_write;
        }

        free(data);
        data = NULL;
    }

    bkpHandle = handle;

    return true;

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
fail_read_file:
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
fail_buf_alloc:
fail_path_too_long:
    return false;
}

int cmpChanges(const void *_chA, const void *_chB)
{
    const struct PatchedBytes *chA;
    const struct PatchedBytes *chB;
    int res;

    assert(_chA);
    assert(_chB);

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
            res = _wcsicmp(chA->fnamePrefix, chB->fnamePrefix);
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

static bool patchData(void)
{
    struct PatchedFile *file;
    const struct PatchedBytes **sortedChanges;
    const struct Section *sec;
    size_t numChanges;
    const wchar_t *fnamePrefix;
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

    // FIXME: Add checks for undefined behavior.

    // Sort the changes on prefix and size.

    numChanges = 0;
    while (changes[numChanges].fnamePrefix) {
        numChanges++;
    }

    // Increment a final time to account for the end-of-array marker.

    numChanges++;

    sortedChanges = _alloca(numChanges * sizeof(struct PatchedBytes *));

    for (ii = 0; ii < numChanges; ii++) {
        sortedChanges[ii] = &changes[ii];
    }

    qsort(sortedChanges, numChanges, sizeof(struct PatchedBytes *), cmpChanges);

    for (ch = sortedChanges; (*ch)->fnamePrefix; ch++) {
        if (!fnamePrefix || _wcsicmp(fnamePrefix, (*ch)->fnamePrefix)) {
            for (file = patchedFiles; file->name; file++) {
                if (!_wcsicmp(file->namePrefix, (*ch)->fnamePrefix)) {
                    break;
                }
            }

            if (!file->name) {
                showError(L"Internal patcher error, failed to find the file to "
                          L"patch for the change set with prefix %s and "
                          L"address 0x%08lX. " TEXT_PLEASE_REPORT,
                          (*ch)->fnamePrefix,
                          (unsigned long) (*ch)->addr);
                goto fail_unknown_file;
            }

            fnamePrefix = (*ch)->fnamePrefix;
            nextByteIdx = 0;
        }

        for (sec = file->sections; sec->name; sec++) {
            // FIXME: undefined behavior checks

            addrFirst = file->imageBase + sec->virtualAddr;
            addrLast = addrFirst + sec->size;

            if ((*ch)->addr >= addrFirst && (*ch)->addr <= addrLast) {
                break;
            }
        }

        if (!sec->name) {
            showError(L"Internal patcher error, failed to find the correct "
                      L"section of file %s for the change set with prefix %s "
                      L"and address 0x%08lX. "
                      TEXT_PLEASE_REPORT,
                      file->name,
                      (*ch)->fnamePrefix,
                      (unsigned long) (*ch)->addr);
            goto fail_unknown_sec;
        }

        firstByteIdx = sec->rawDataPtr
                       + ((*ch)->addr - sec->virtualAddr - file->imageBase);

        if (firstByteIdx < nextByteIdx) {
            showError(L"Internal patcher error, at least two change sets for "
                      L"the file %s (change prefix %s) overlap. "
                      TEXT_PLEASE_REPORT,
                      file->nameUppercase,
                      fnamePrefix);
            goto fail_overlapping_secs;
        }

        // Apply the changes to the data.

        nextByteIdx = firstByteIdx;
        bytes = (*ch)->bytes;
        bytesLen = strlen(bytes);

        ii = 0;
        while (ii < bytesLen) {
            if (!isspace(bytes[ii])) {
                if (ii == bytesLen - 1
                    || (hexValA = hexDigitToNum(bytes[ii])) == -1
                    || (hexValB = hexDigitToNum(bytes[ii + 1])) == -1) {
                    showError(L"Internal patcher error, invalid byte "
                              L"representation for the change set with "
                              L"prefix %s and address 0x%08lX. "
                              TEXT_PLEASE_REPORT,
                              (*ch)->fnamePrefix,
                              (unsigned long) (*ch)->addr);
                    goto fail_invalid_hex_str;
                }

                if (nextByteIdx == file->size) {
                    showError(L"Internal patcher error, change set with prefix "
                              L"%s and address extends past the end of the "
                              L"file %s. ",
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

        if (nextByteIdx != firstByteIdx) {
            file->modified = true;
        }
    }

    return true;

fail_invalid_byte_addr:
fail_invalid_hex_str:
fail_overlapping_secs:
fail_unknown_sec:
fail_unknown_file:
    return false;
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prevInst, LPSTR cmdLine,
                   int nShowCmd)
{
    struct PatchedFile *file;

    pathSep = L"\\";
    pathSepLen = wcslen(pathSep);

    if (!(parentPath = getPatcherParentPath())) {
        goto fail_parent;
    }

    parentPathLen = wcslen(parentPath);

    for (file = patchedFiles; file->name; file++) {
        if (!readFile(file)) {
            goto fail_read_file;
        }
    }

    if (allFilesPatched) {
        showInfo(L"Nothing will be done, the game files are already patched.");
        goto all_patched;
    }

    if (!createBackup()) {
        goto fail_backup;
    }

    if (!patchData()) {
        goto fail_patch;
    }

    for (file = patchedFiles; file->name; file++) {
        if (file->modified && !writeFile(file)) {
            goto fail_write_file;
        }
    }

    showInfo(L"Patching was successful. A number of games files were modified "
             L"and a backup file %s containing the originals was created in "
             L"the same directory. If you wish to uninstall the patch just "
             L"the contents of the backup in the game directory. Enjoy "
             L"Hitman: Codename 47!", bkpFilenameUppercase);

all_patched:
    CloseHandle(bkpHandle);
    closeFiles();
    free(parentPath);
    return 0;

fail_write_file:
fail_patch:
    CloseHandle(bkpHandle);
fail_backup:
fail_read_file:
    closeFiles();
    free(parentPath);
fail_parent:
    return 1;
}

