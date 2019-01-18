/*
 * Copyright 2014 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

using System;
using System.Collections.Generic;

namespace FlatBuffers
{
    /// <summary>
    /// Offset class for typesafe assignments.
    /// </summary>
    public struct Offset<T> where T : struct
    {
        public int Value;
        public Offset(int value)
        {
            Value = value;
        }
    }

    public struct StringOffset
    {
        public int Value;
        public StringOffset(int value)
        {
            Value = value;
        }
    }

    public struct VectorOffset
    {
        public int Value;
        public VectorOffset(int value)
        {
            Value = value;
        }
    }

    public class FlatAttribute : Attribute
    {
        Dictionary<string, object> custom;
        public FlatAttribute(Dictionary<string, object> custom = null)
        {
            this.custom = custom;
        }
    }

    public class FlatFieldAttribute : FlatAttribute
    {
        int id; //table field
        bool deprecated; //field
        bool required; //non-scalar table field
        bool key;//field

        public FlatFieldAttribute(int id = -1, bool deprecated = false, bool required = false, bool key = false, Dictionary<string, object> custom = null) : base(custom)
        {
            this.id = id;
            this.deprecated = deprecated;
            this.required = required;
            this.key = key;
        }
    }

    public class FlatEnumAttribute : FlatAttribute
    {
        int bitFlag;
        public FlatEnumAttribute(int bitFlag = 1, Dictionary<string, object> custom = null) : base(custom)
        {
            this.bitFlag = bitFlag;
        }
    }

    public class FlatTableAttribute : FlatAttribute
    {
        bool originalOrder;
        public FlatTableAttribute(bool originalOrder, Dictionary<string, object> custom = null) : base(custom)
        {
            this.originalOrder = originalOrder;
        }
    }

    public class FlatStructAttribute : FlatAttribute
    {
        int forceAlign;
        public FlatStructAttribute(int forceAlign = 0, Dictionary<string, object> custom = null) : base(custom)
        {
            this.forceAlign = forceAlign;
        }
    }
}
