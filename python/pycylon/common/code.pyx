##
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 # http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
 ##

from libcpp.string cimport string
from libcpp cimport bool
from pycylon.common.code cimport _Code

'''
Cylon C++ Error Tracing is done with the following enums. 
'''

cpdef enum Code:
    OK = _Code._OK
    OutOfMemory = _Code._OutOfMemory
    KeyError = _Code._KeyError
    TypeError = _Code._TypeError
    Invalid = _Code._Invalid
    IOError = _Code._IOError
    CapacityError = _Code._CapacityError
    IndexError = _Code._IndexError
    UnknownError = _Code._UnknownError
    NotImplemented = _Code._NotImplemented
    SerializationError = _Code._SerializationError
    RError = _Code._RError
    CodeGenError = _Code._CodeGenError
    ExpressionValidationError = _Code._ExpressionValidationError
    ExecutionError = _Code._ExecutionError
    AlreadyExists = _Code._AlreadyExists


