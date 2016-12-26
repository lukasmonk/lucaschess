//  GREKO Chess Engine
//  (c) 2002-2012 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.110mb.com

//  config.cpp: configuration file code
//  modified: 31-Dec-2012

#pragma once

#include "types.h"

class Config
{
public:

  Config(const std::string path)
  {
    FILE* file = fopen(path.c_str(), "rt");
    if (file == NULL)
      return;
    char buf[256];
    while (fgets(buf, sizeof(buf), file))
    {
      if (buf[strlen(buf) - 1] == '\n' || buf[strlen(buf) - 1] == '\r')
        buf[strlen(buf) - 1] = 0;

      std::string s(buf);
      size_t pos = s.find("=");
      if (pos != std::string::npos && pos < s.length() - 1)
      {
        std::string key = s.substr(0, pos);
        std::string value = s.substr(pos + 1, s.length() - pos - 1);
        m_data[key] = value;
      }
    }
    fclose(file);
  }

  int GetInt(const std::string& key, int defaultValue)
  {
    std::map<std::string, std::string>::const_iterator it = m_data.find(key);
    if (it == m_data.end())
      return defaultValue;
    else
      return atoi(it->second.c_str());
  }

  double GetDouble(const std::string& key, double defaultValue)
  {
    std::map<std::string, std::string>::const_iterator it = m_data.find(key);
    if (it == m_data.end())
      return defaultValue;
    else
      return atof(it->second.c_str());
  }

private:

  std::map<std::string, std::string> m_data;
};
