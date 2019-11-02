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
    PVarType field(PVarType::VarDynamicString);
    int result = ReadField(channel, field, false);
    if (result < 0)
      return false;

    PString heading = field.AsString();
    if (heading.IsEmpty())
      return false;

    m_headings.AppendString(heading);
    if (result > 0)
      return true;
  }
}


bool PTextDataFormat::WriteHeadings(PChannel & channel)
{
  if (!PAssert(!m_headings.IsEmpty(), PInvalidParameter))
    return false;

  PINDEX last = m_headings.GetSize() - 1;
  for (PINDEX i = 0; i <= last; ++i) {
    if (!WriteField(channel, m_headings[i], i == last))
      return false;
  }

  return true;
}


bool PTextDataFormat::ReadRecord(PChannel & channel, PVarData::Record & data)
{
  for (PINDEX i = 0; i < m_headings.GetSize(); ++i) {
    PString heading = m_headings[i];

    bool autoDetect = false;
    PVarType * field = data.GetAt(heading);
    if (field == NULL) {
      field = new PVarType;
      autoDetect = true;
    }

    int result = ReadField(channel, *field, autoDetect);
    data.SetAt(heading, field);

    if (result != 0)
      return result > 0;
  }

  for (;;) {
    PVarType dummy;
    int result = ReadField(channel, dummy, false);
    if (result != 0)
      return result > 0;
  }
}


bool PTextDataFormat::WriteRecord(PChannel & channel, const PVarData::Record & data)
{
  if (!PAssert(!m_headings.IsEmpty(), PInvalidParameter))
    return false;

  PINDEX last = m_headings.GetSize() - 1;
  for (PINDEX i = 0; i <= last; ++i) {
    PVarType * field = data.GetAt((m_headings[i]));
    if (!WriteField(channel, field != NULL ? *field : PVarType(), i == last))
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


int PCommaSeparatedVariableFormat::ReadField(PChannel & channel, PVarType & field, bool autoDetect)
{
  bool inQuote = false;
  bool wasQuoted = false;
  bool escaped = false;
  bool skipws = true;

  PStringStream str;
  while (channel.good()) {
    int c = channel.get();

    if (inQuote) {
      if (c == '"')
        inQuote = false;
      else if (escaped) {
        str << (char)c;
        escaped = false;
      }
      else if (c == '\\')
        escaped = true;
      else
        str << (char)c;
    }
    else {
      if (c == '\r' || c == '\n' || c == ',') {
        if (autoDetect && wasQuoted)
          field.SetDynamicString(str);
        else
          field.FromString(str, autoDetect);
        return c == ',' ? 0 : 1;
      }

      if (skipws && isblank(c))
        continue;
      skipws = false;

      if (c == '"')
        inQuote = wasQuoted = true;
      else
        str << (char)c;
    }
  }

  return -1;
}


bool PCommaSeparatedVariableFormat::WriteField(PChannel & channel, const PVarType & field, bool endOfLine)
{
  switch (field.GetType()) {
    case PVarType::VarStaticString:
    case PVarType::VarDynamicString:
      channel << field.AsString().ToLiteral();
      break;
    case PVarType::VarStaticBinary :
    case PVarType::VarDynamicBinary :
      channel << field.AsString();
      break;
    default:
      channel << field;
  }
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


int PTabDelimitedFormat::ReadField(PChannel & channel, PVarType & field, bool autoDetect)
{
  PStringStream str;
  while (channel.good()) {
    int c = channel.get();
    if (c == '\r' || c == '\n' || c == '\t') {
      field.FromString(str, autoDetect);
      return c == '\t' ? 0 : 1;
    }

    str << (char)c;
  }

  return -1;
}


bool PTabDelimitedFormat::WriteField(PChannel & channel, const PVarType & field, bool endOfLine)
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
  , m_needToWriteHeadings(true)
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


bool PTextDataFile::ReadRecord(PVarData::Record & data)
{
  if (CheckNotOpen())
    return false;

  data.RemoveAll();

  m_formatting = true;
  bool ok = m_format->ReadRecord(*this, data);
  m_formatting = false;
  return ok;
}


bool PTextDataFile::WriteRecord(const PVarData::Record & data)
{
  if (CheckNotOpen())
    return false;

  m_formatting = true;
  if (m_needToWriteHeadings) {
    if (m_format->GetHeadings().IsEmpty())
      m_format->SetHeadings(data.GetKeys());
    if (!m_format->WriteHeadings(*this))
      return false;
    m_needToWriteHeadings = false;
  }
  bool ok = m_format->WriteRecord(*this, data);
  m_formatting = false;
  return ok;
}


bool PTextDataFile::ReadObject(PVarData::Object & obj)
{
  if (CheckNotOpen())
    return false;

  m_formatting = true;
  bool ok = m_format->ReadRecord(*this, obj.GetMemberValues());
  m_formatting = false;
  return ok;
}


bool PTextDataFile::WriteObject(const PVarData::Object & obj)
{
  if (CheckNotOpen())
    return false;

  m_formatting = true;
  if (m_needToWriteHeadings) {
    m_format->SetHeadings(obj.GetMemberNames());
    if (!m_format->WriteHeadings(*this))
      return false;
    m_needToWriteHeadings = false;
  }
  bool ok = m_format->WriteRecord(*this, obj.GetMemberValues());
  m_formatting = false;
  return ok;
}


bool PTextDataFile::InternalOpen(OpenMode mode, OpenOptions opts, PFileInfo::Permissions permissions)
{
  if (!PAssert(mode != ReadWrite, PInvalidParameter))
    return false;

  if (m_format.IsNULL()) {
    if (GetFilePath().GetType() == ".csv")
      m_format = new PCommaSeparatedVariableFormat();
    else if (GetFilePath().GetType() == ".txt" || GetFilePath().GetType() == ".tsv")
      m_format = new PTabDelimitedFormat();
    else
      return false;
  }

  if (!PTextFile::InternalOpen(mode, opts, permissions))
    return false;

  if (mode == WriteOnly) {
    m_needToWriteHeadings = true;
    return true;
  }

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
