/**
	data-chan physic through USB
	Copyright (C) 2016  Benato Denis

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef NODEVICEEXCEPTION_HPP
#define NODEVICEEXCEPTION_HPP

#include <cstddef>
#include <exception>

#ifdef __cplusplus
extern "C" {
#endif

namespace DataChan {
    class NoDeviceException : public std::exception {
    public:
        NoDeviceException();
        const char* what() noexcept;
    };
}

#ifdef __cplusplus
}
#endif

#endif /* NODEVICEEXCEPTION_HPP */
