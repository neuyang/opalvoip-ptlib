/*
 * $Id: ptlib.cxx,v 1.15 1995/04/25 11:33:35 robertj Exp $
 *
 * Portable Windows Library
 *
 * Operating System Classes Implementation
 *
 * Copyright 1993 by Robert Jongbloed and Craig Southeren
 *
 * $Log: ptlib.cxx,v $
 * Revision 1.15  1995/04/25 11:33:35  robertj
 * Changes for DLL support.
 *
 * Revision 1.14  1995/04/22 00:53:49  robertj
 * Added Move() function to PFile.
 * Changed semantics of Rename() function in PFile.
 * Changed file path string to use PFilePath object.
 *
 * Revision 1.13  1995/03/12 05:00:08  robertj
 * Re-organisation of DOS/WIN16 and WIN32 platforms to maximise common code.
 * Used built-in equate for WIN32 API (_WIN32).
 *
 * Revision 1.12  1995/02/27  10:37:06  robertj
 * Added GetUserNmae().
 * Removed superfluous Read() and Write() for text files.
 *
 * Revision 1.11  1995/01/06  10:41:43  robertj
 * Moved identifiers into different scope.
 * Changed file size to 64 bit integer.
 *
 * Revision 1.10  1994/12/21  11:36:07  robertj
 * Fixed caseless string for file paths.
 *
 * Revision 1.9  1994/10/30  11:26:54  robertj
 * Fixed set current directory function.
 * Changed PFilePath to be case insignificant according to platform.
 *
 * Revision 1.8  1994/10/23  05:42:39  robertj
 * PipeChannel headers.
 * ConvertOSError function added.
 * Numerous implementation enhancements.
 *
 * Revision 1.7  1994/08/04  13:24:27  robertj
 * Added debug stream.
 *
 * Revision 1.6  1994/07/27  06:00:10  robertj
 * Backup
 *
 * Revision 1.5  1994/07/21  12:35:18  robertj
 * *** empty log message ***
 *
 * Revision 1.4  1994/07/17  11:01:04  robertj
 * Ehancements, implementation, bug fixes etc.
 *
 * Revision 1.3  1994/07/02  03:18:09  robertj
 * Multi-threading implementation.
 *
 * Revision 1.2  1994/06/25  12:13:01  robertj
 * Synchronisation.
 *
// Revision 1.1  1994/04/01  14:39:35  robertj
// Initial revision
//
 */

#include <ptlib.h>

#include <errno.h>
#include <fcntl.h>
#include <sys\stat.h>

#ifndef P_USE_INLINES
#include <osutil.inl>
#include <ptlib.inl>
#endif


#if defined(_WIN32) && defined(_WINDLL)

__declspec(dllexport) PProcess * PProcessInstance;
__declspec(dllexport) ostream * PErrorStream;

#endif

const PTimeInterval PMaxTimeInterval = 0x7fffffff;


///////////////////////////////////////////////////////////////////////////////
// PChannel

void PChannel::Construct()
{
}


PString PChannel::GetName() const
{
  PAssertAlways(PUnimplementedFunction);
  return PString();
}


BOOL PChannel::Read(void *, PINDEX)
{
  PAssertAlways(PUnimplementedFunction);
  return FALSE;
}


BOOL PChannel::Write(const void *, PINDEX)
{
  PAssertAlways(PUnimplementedFunction);
  return FALSE;
}


BOOL PChannel::Close()
{
  PAssertAlways(PUnimplementedFunction);
  return FALSE;
}


PString PChannel::GetErrorText() const
{
  if (osError == 0)
    return PString();

  if (osError > 0 && osError < _sys_nerr) {
    if (_sys_errlist[osError][0] != '\0')
      return _sys_errlist[osError];
  }

  return psprintf("OS error %d", osError);
}


BOOL PChannel::ConvertOSError(int error)
{
  if (error >= 0)
    return TRUE;

#if defined(_WIN32)
  if (error != -2)
    osError = errno;
  else {
    osError = GetLastError();
    switch (osError) {
      case ERROR_INVALID_HANDLE :
        osError = EBADF;
        break;
      case ERROR_INVALID_PARAMETER :
        osError = EINVAL;
        break;
      case ERROR_ACCESS_DENIED :
        osError = EACCES;
        break;
      case ERROR_NOT_ENOUGH_MEMORY :
        osError = ENOMEM;
        break;
      default :
        osError |= 0x40000000;
    }
  }
#endif

  switch (osError) {
    case 0 :
      lastError = NoError;
      return TRUE;
    case ENOENT :
      lastError = NotFound;
      break;
    case EEXIST :
      lastError = FileExists;
      break;
    case EACCES :
      lastError = AccessDenied;
      break;
    case ENOMEM :
      lastError = NoMemory;
      break;
    case ENOSPC :
      lastError = DiskFull;
      break;
    case EINVAL :
      lastError = BadParameter;
      break;
    case EBADF :
      lastError = NotOpen;
      break;
    default :
      lastError = Miscellaneous;
  }

  return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
// Directories

void PDirectory::CopyContents(const PDirectory & dir)
{
  scanMask = dir.scanMask;
  fileinfo = dir.fileinfo;
}


BOOL PDirectory::Change(const PString & p)
{
  PDirectory d = p;
  return _chdrive(toupper(d[0])-'A'+1) == 0 && _chdir(d + ".") == 0;
}


BOOL PDirectory::Filtered()
{
#if defined(_WIN32)
  char * name = fileinfo.cFileName;
#else
  char * name = fileinfo.name;
#endif
  if (strcmp(name, ".") == 0)
    return TRUE;
  if (strcmp(name, "..") == 0)
    return TRUE;
  if (scanMask == PFileInfo::AllPermissions)
    return FALSE;

  PFileInfo inf;
  PAssert(PFile::GetInfo(*this+name, inf), POperatingSystemError);
  return (inf.type&scanMask) == 0;
}


BOOL PDirectory::IsRoot() const
{
  return GetLength() == 3;
}


///////////////////////////////////////////////////////////////////////////////
// File Path

PFilePath::PFilePath(const PString & str)
  : PCaselessString(PDirectory::CreateFullPath(str, FALSE))
{
}


PFilePath::PFilePath(const char * cstr)
  : PCaselessString(PDirectory::CreateFullPath(cstr, FALSE))
{
}


PFilePath::PFilePath(const char * prefix, const char * dir)
{
  if (dir != NULL) {
    PDirectory tmpdir(dir);
    operator=(tmpdir);
  }
  else {
    PConfig cfg(PConfig::Environment);
    PString path = cfg.GetString("TMPDIR");
    if (path.IsEmpty()) {
      path = cfg.GetString("TMP");
      if (path.IsEmpty())
        path = cfg.GetString("TEMP");
    }
    if (path.IsEmpty() || path[path.GetLength()-1] != '\\')
      path += '\\';
    *this = path;
  }
  if (prefix != NULL)
    *this += prefix;
  else
    *this += "PW";
  *this += "XXXXXX";
  PAssert(_mktemp(GetPointer()) != NULL, "Could not make temporary file");
}


PFilePath & PFilePath::operator=(const PString & str)
{
  PCaselessString::operator=(PDirectory::CreateFullPath(str, FALSE));
  return *this;
}


PCaselessString PFilePath::GetVolume() const
{
  PINDEX colon = Find(':');
  if (colon == P_MAX_INDEX)
    return PCaselessString();
  return Left(colon+1);
}


PCaselessString PFilePath::GetPath() const
{
  PINDEX backslash = FindLast('\\');
  if (backslash == P_MAX_INDEX)
    return PCaselessString();

  PINDEX colon = Find(':');
  if (colon == P_MAX_INDEX)
    colon = 0;
  else
    colon++;

  return operator()(colon, backslash);
}


PCaselessString PFilePath::GetFileName() const
{
  PINDEX backslash = FindLast('\\');
  if (backslash == P_MAX_INDEX)
    backslash = 0;
  else
    backslash++;

  return Mid(backslash);
}


PCaselessString PFilePath::GetTitle() const
{
  PINDEX backslash = FindLast('\\');
  if (backslash == P_MAX_INDEX)
    backslash = 0;
  else
    backslash++;

  return operator()(backslash, FindLast('.')-1);
}


PCaselessString PFilePath::GetType() const
{
  PINDEX dot = FindLast('.');
  if (dot == P_MAX_INDEX)
    return PCaselessString();
  return operator()(dot, P_MAX_INDEX);
}


void PFilePath::SetType(const PCaselessString & type)
{
  PINDEX dot = FindLast('.');
  if (dot != P_MAX_INDEX)
    *this = Left(dot-1) + type;
  else
    *this += type;
}


///////////////////////////////////////////////////////////////////////////////
// PFile

void PFile::CopyContents(const PFile & f)
{
  path = f.path;
  os_handle = f.os_handle;
}


void PFile::SetFilePath(const PString & newName)
{
  if (!IsOpen())
    path = newName;
}


BOOL PFile::Access(const PFilePath & name, OpenMode mode)
{
  int accmode;

  switch (mode) {
    case ReadOnly :
      accmode = 2;
      break;

    case WriteOnly :
      accmode = 4;
      break;

    default :
      accmode = 6;
  }

  return access(name, accmode) == 0;
}


BOOL PFile::Remove(const PFilePath & name, BOOL force)
{
  if (remove(name) == 0)
    return TRUE;
  if (!force || errno != EACCES)
    return FALSE;
  if (_chmod(name, _S_IWRITE) != 0)
    return FALSE;
  return remove(name) == 0;
}


BOOL PFile::Rename(const PFilePath & oldname, const PString & newname, BOOL force)
{
  if (newname.FindOneOf(":\\/") != P_MAX_INDEX) {
    errno = EINVAL;
    return FALSE;
  }
  if (rename(oldname, oldname.GetDirectory() + newname) == 0)
    return TRUE;
  if (!force || errno == ENOENT || !Exists(newname))
    return FALSE;
  if (!Remove(newname, TRUE))
    return FALSE;
  return rename(oldname, oldname.GetDirectory() + newname) == 0;
}


BOOL PFile::Move(const PFilePath & oldname, const PFilePath & newname, BOOL force)
{
  if (rename(oldname, newname) == 0)
    return TRUE;
  if (errno == ENOENT)
    return FALSE;
  if (force && Exists(newname)) {
    if (!Remove(newname, TRUE))
      return FALSE;
    if (rename(oldname, newname) == 0)
      return TRUE;
  }
  return Copy(oldname, newname, force) && Remove(oldname);
}


BOOL PFile::GetInfo(const PFilePath & name, PFileInfo & info)
{
  struct stat s;
  if (stat(name, &s) != 0)
    return FALSE;

  info.created =  (s.st_ctime < 0) ? 0 : s.st_ctime;
  info.modified = (s.st_mtime < 0) ? 0 : s.st_mtime;
  info.accessed = (s.st_atime < 0) ? 0 : s.st_atime;
  info.size = s.st_size;

  info.permissions = 0;
  if ((s.st_mode&S_IREAD) != 0)
    info.permissions |=
                PFileInfo::UserRead|PFileInfo::GroupRead|PFileInfo::WorldRead;
  if ((s.st_mode&S_IWRITE) != 0)
    info.permissions |=
             PFileInfo::UserWrite|PFileInfo::GroupWrite|PFileInfo::WorldWrite;
  if ((s.st_mode&S_IEXEC) != 0)
    info.permissions |=
       PFileInfo::UserExecute|PFileInfo::GroupExecute|PFileInfo::WorldExecute;

  switch (s.st_mode & S_IFMT) {
    case S_IFREG :
      info.type = PFileInfo::RegularFile;
      break;

    case S_IFDIR :
      info.type = PFileInfo::SubDirectory;
      break;

    default:
      info.type = PFileInfo::UnknownFileType;
      break;
  }

#if defined(_WIN32)
  info.hidden = (GetFileAttributes(name) & FILE_ATTRIBUTE_HIDDEN) != 0;
#else
  unsigned int attr;
  _dos_getfileattr(name, &attr);
  info.hidden = (attr & _A_HIDDEN) != 0;
#endif

  return TRUE;
}


BOOL PFile::SetPermissions(const PFilePath & name, int permissions)
{
  return _chmod(name, permissions&(_S_IWRITE|_S_IREAD)) == 0;
}


BOOL PFile::IsTextFile() const
{
  return FALSE;
}


BOOL PFile::Open(OpenMode mode, int opts)
{
  Close();
  clear();

  if (path.IsEmpty())
    path = PFilePath("PWL", NULL);

  int oflags = IsTextFile() ? _O_TEXT : _O_BINARY;
  switch (mode) {
    case ReadOnly :
      oflags |= O_RDONLY;
      if (opts == ModeDefault)
        opts = MustExist;
      break;
    case WriteOnly :
      oflags |= O_WRONLY;
      if (opts == ModeDefault)
        opts = Create|Truncate;
      break;
    default :
      oflags |= O_RDWR;
      if (opts == ModeDefault)
        opts = Create;
  }

  if ((opts&Create) != 0)
    oflags |= O_CREAT;
  if ((opts&Exclusive) != 0)
    oflags |= O_EXCL;
  if ((opts&Truncate) != 0)
    oflags |= O_TRUNC;

  if ((opts&Temporary) != 0)
    removeOnClose = TRUE;

  return ConvertOSError(os_handle = _open(path, oflags, S_IREAD|S_IWRITE));
}


BOOL PFile::SetLength(off_t len)
{
  return ConvertOSError(_chsize(GetHandle(), len));
}


///////////////////////////////////////////////////////////////////////////////
// PTextFile

BOOL PTextFile::IsTextFile() const
{
  return TRUE;
}


BOOL PTextFile::ReadLine(PString & str)
{
  char * ptr = str.GetPointer(100);
  PINDEX len = 0;
  int c;
  while ((c = ReadChar()) >= 0 && c != '\n') {
    *ptr++ = (char)c;
    if (++len >= str.GetSize())
      ptr = str.GetPointer(len + 100) + len;
  }
  *ptr = '\0';
  PAssert(str.MakeMinimumSize(), POutOfMemory);
  return c >= 0 || len > 0;
}


BOOL PTextFile::WriteLine(const PString & str)
{
  return WriteString(str) && WriteChar('\n');
}



// End Of File ///////////////////////////////////////////////////////////////
