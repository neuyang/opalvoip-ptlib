/*
* textdata.cxx
*
* Text data stream parser
*
* Portable Tools Library
*
* Copyright (C) 2019 by Vox Lucida Pty. Ltd.
*
* The contents of this file are subject to the Mozilla Public License
* Version 1.0 (the "License"); you may not use this file except in
* compliance with the License. You may obtain a copy of the License at
* http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS"
* basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
* the License for the specific language governing rights and limitations
* under the License.
*
* The Original Code is Portable Tools Library.
*
* The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
*
* Contributor(s): Robert Jongbloed
*                 Blackboard, inc
*/

#include <ptlib.h>

#include <ptclib/textdata.h>


PTextDataFormat::PTextDataFormat()
{
}


PTextDataFormat::PTextDataFormat(const PStringArray & headings)
  : m_headings(headings)
{
}


bool PTextDataFormat::ReadHeadings(PChannel & channel)
{
  m_headings.RemoveAll();

  for (;;) {
    PString heading;
    int result = ReadField(channel, heading);
    if (result < 0)
      return false;

    if (heading.IsEmpty())
      return false;

    m_headings.AppendString(heading);
    if (result > 0)
      return true;
  }
}


bool PTextDataFormat::WriteHeadings(PChannel & channel)
{
  if (!PAssert(m_headings.IsEmpty(), PInvalidParameter))
    return false;

  PINDEX last = m_headings.GetSize() - 1;
  for (PINDEX i = 0; i <= last; ++i) {
    if (!WriteField(channel, m_headings[i], i == last))
      return false;
  }

  return true;
}


bool PTextDataFormat::ReadRecord(PChannel & channel, PStringToString & data)
{
  data.RemoveAll();

  PINDEX i = 0;
  for (;;) {
    PString field;
    int result = ReadField(channel, field);
    if (i < m_headings.GetSize())
      data.SetAt(m_headings[i++], field);
    if (result != 0)
      return result > 0;
  }
}


bool PTextDataFormat::WriteRecord(PChannel & channel, const PStringToString & data)
{
  if (!PAssert(m_headings.IsEmpty(), PInvalidParameter))
    return false;

  PINDEX last = m_headings.GetSize() - 1;
  for (PINDEX i = 0; i <= last; ++i) {
    if (!WriteField(channel, data(m_headings[i]), i == last))
      return false;
  }

  return true;
}


PCommaSeparatedVariableFormat::PCommaSeparatedVariableFormat()
{
}


PCommaSeparatedVariableFormat::PCommaSeparatedVariableFormat(const PStringArray & headings)
  : PTextDataFormat(headings)
{
}


int PCommaSeparatedVariableFormat::ReadField(PChannel & channel, PString & field)
{
  bool inQuote = false;
  bool escaped = false;
  bool skipws = true;

  while (channel.good()) {
    int c = channel.get();

    if (inQuote) {
      if (c == '"')
        inQuote = false;
      else if (escaped) {
        field += (char)c;
        escaped = false;
      }
      else if (c == '\\')
        escaped = true;
      else
        field += (char)c;
    }
    else {
      if (c == '\r' || c == '\n')
        return 1;

      if (c == ',')
        return 0;

      if (skipws && isblank(c))
        continue;
      skipws = false;

      if (c == '"')
        inQuote = true;
      else
        field += (char)c;
    }
  }

  return -1;
}


bool PCommaSeparatedVariableFormat::WriteField(PChannel & channel, const PString & field, bool endOfLine)
{
  channel << field.ToLiteral();
  if (endOfLine)
    channel << endl;
  else
    channel << ',';
  return channel.good();
}


PTabDelimitedFormat::PTabDelimitedFormat()
{
}


PTabDelimitedFormat::PTabDelimitedFormat(const PStringArray & headings)
  : PTextDataFormat(headings)
{
}


int PTabDelimitedFormat::ReadField(PChannel & channel, PString & field)
{
  while (channel.good()) {
    int c = channel.get();
    if (c == '\r' || c == '\n')
      return 1;

    if (c == '\t')
      return 0;

    field += (char)c;
  }

  return -1;
}


bool PTabDelimitedFormat::WriteField(PChannel & channel, const PString & field, bool endOfLine)
{
  channel << field;
  if (endOfLine)
    channel << endl;
  else
    channel << '\t';
  return channel.good();
}


PTextDataFile::PTextDataFile(PTextDataFormatPtr format)
  : m_format(format)
  , m_formatting(false)
{
}


PTextDataFile::PTextDataFile(const PFilePath & name, OpenMode mode, OpenOptions opts, PTextDataFormatPtr format)
  : m_format(format)
  , m_formatting(false)
{
  Open(name, mode, opts);
}


bool PTextDataFile::SetFormat(const PTextDataFormatPtr & format)
{
  if (IsOpen() || format.IsNULL())
    return false;

  m_format = format;
  return true;
}


bool PTextDataFile::ReadRecord(PStringToString & data)
{
  if (CheckNotOpen())
    return false;

  m_formatting = true;
  bool ok = m_format->ReadRecord(*this, data);
  m_formatting = false;
  return ok;
}


bool PTextDataFile::WriteRecord(const PStringToString & data)
{
  if (CheckNotOpen())
    return false;

  m_formatting = true;
  bool ok = m_format->WriteRecord(*this, data);
  m_formatting = false;
  return ok;
}


bool PTextDataFile::InternalOpen(OpenMode mode, OpenOptions opts, PFileInfo::Permissions permissions)
{
  if (!PAssert(mode != ReadWrite, PInvalidParameter))
    return false;

  if (mode == ReadOnly && m_format.IsNULL() && GetFilePath().GetType() == ".csv")
    m_format = new PCommaSeparatedVariableFormat();
  else if (!PAssert(mode == ReadOnly || !m_format.IsNULL(), PInvalidParameter))
    return false;

  if (!PTextFile::InternalOpen(mode, opts, permissions))
    return false;

  if (mode == WriteOnly)
    return m_format->WriteHeadings(*this);

  m_formatting = true;

  bool ok = false;

  if (m_format.IsNULL()) {
    bool hadComma = false;
    for (;;) {
      char c;
      if (!Read(&c, 1))
        break;

      if (c == '\t') {
        SetPosition(0);
        m_format = new PTabDelimitedFormat;
        break;
      }

      if (c == '\n') {
        SetPosition(0);
        break;
      }

      if (c == ',')
        hadComma = true;
    }

    if (m_format.IsNULL()) {
      if (!hadComma)
        return false;
      m_format = new PCommaSeparatedVariableFormat();
    }
  }
  else
    ok = !m_format->GetHeadings().IsEmpty();

  if (!ok)
    ok = m_format->ReadHeadings(*this);

  m_formatting = false;
  return ok;
}


PBoolean PTextDataFile::Read(void * buf, PINDEX len)
{
  return m_formatting && PTextFile::Read(buf, len);
}


int PTextDataFile::ReadChar()
{
  return -1;
}


PBoolean PTextDataFile::ReadBlock(void *, PINDEX)
{
  return false;
}


PString PTextDataFile::ReadString(PINDEX)
{
  return PString::Empty();
}


PBoolean PTextDataFile::Write(const void * buf, PINDEX len)
{
  return m_formatting && PTextFile::Write(buf, len);
}


PBoolean PTextDataFile::WriteChar(int)
{
  return false;
}


PBoolean PTextDataFile::WriteString(const PString &)
{
  return false;
}
