/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef LOGMACROS_H
#define LOGMACROS_H

#include "Logger.h"
#include <QString>

namespace TR4QT {

// Convenience macros for categorized logging
#define LOG_TRACE(category, msg) \
    TR4QT::Logger::instance().log(TR4QT::LogLevel::Trace, category, msg, __FILE__, __LINE__)

#define LOG_DEBUG(category, msg) \
    TR4QT::Logger::instance().log(TR4QT::LogLevel::Debug, category, msg, __FILE__, __LINE__)

#define LOG_INFO(category, msg) \
    TR4QT::Logger::instance().log(TR4QT::LogLevel::Info, category, msg, __FILE__, __LINE__)

#define LOG_WARN(category, msg) \
    TR4QT::Logger::instance().log(TR4QT::LogLevel::Warn, category, msg, __FILE__, __LINE__)

#define LOG_ERROR(category, msg) \
    TR4QT::Logger::instance().log(TR4QT::LogLevel::Error, category, msg, __FILE__, __LINE__)

#define LOG_FATAL(category, msg) \
    TR4QT::Logger::instance().log(TR4QT::LogLevel::Fatal, category, msg, __FILE__, __LINE__)

// Printf-style formatting helpers
#define LOG_TRACE_F(category, ...) \
    LOG_TRACE(category, QString::asprintf(__VA_ARGS__))

#define LOG_DEBUG_F(category, ...) \
    LOG_DEBUG(category, QString::asprintf(__VA_ARGS__))

#define LOG_INFO_F(category, ...) \
    LOG_INFO(category, QString::asprintf(__VA_ARGS__))

#define LOG_WARN_F(category, ...) \
    LOG_WARN(category, QString::asprintf(__VA_ARGS__))

#define LOG_ERROR_F(category, ...) \
    LOG_ERROR(category, QString::asprintf(__VA_ARGS__))

#define LOG_FATAL_F(category, ...) \
    LOG_FATAL(category, QString::asprintf(__VA_ARGS__))

} // namespace TR4QT

#endif // LOGMACROS_H
