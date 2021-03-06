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

#include <iostream>
#include "flatbuffers/code_generators.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"

namespace flatbuffers {

static std::string GeneratedFileName(const std::string &path,
                                     const std::string &file_name) {
  return path + file_name + ".schema.json";
}

namespace jsons {

std::string GenNativeType(BaseType type) {
  switch (type) {
    case BASE_TYPE_BOOL: return "boolean";
    case BASE_TYPE_CHAR:
    case BASE_TYPE_UCHAR:
    case BASE_TYPE_SHORT:
    case BASE_TYPE_USHORT:
    case BASE_TYPE_INT:
    case BASE_TYPE_UINT:
    case BASE_TYPE_LONG:
    case BASE_TYPE_ULONG: return "integer";
    case BASE_TYPE_FLOAT:
    case BASE_TYPE_DOUBLE: return "number";
    case BASE_TYPE_STRING: return "string";
    default: return "";
  }
}

template<class T> std::string GenFullName(const T *enum_def) {
  std::string full_name;
  const auto &name_spaces = enum_def->defined_namespace->components;
  for (auto ns = name_spaces.cbegin(); ns != name_spaces.cend(); ++ns) {
    full_name.append(*ns + "_");
  }
  full_name.append(enum_def->name);
  return full_name;
}

template<class T> std::string GenTypeRef(const T *enum_def, const char* suffix="") {
  return "\"$ref\" : \"#/definitions/" + GenFullName(enum_def) + suffix + "\"";
}

struct IntegerInfo {
  BaseType type;
  std::int64_t minValue;
  std::uint64_t maxValue;
  const char* name() const {
    return kTypeNames[type];
  }
} IntegerInfos[] = {
  { BaseType::BASE_TYPE_CHAR, std::numeric_limits<std::int8_t>::min(), (std::uint64_t)std::numeric_limits<std::int8_t>::max() },
  { BaseType::BASE_TYPE_UCHAR, std::numeric_limits<std::uint8_t>::min(), std::numeric_limits<std::uint8_t>::max() },

  { BaseType::BASE_TYPE_SHORT, std::numeric_limits<std::int16_t>::min(), (std::uint64_t)std::numeric_limits<std::int16_t>::max() },
  { BaseType::BASE_TYPE_USHORT, std::numeric_limits<std::uint16_t>::min(), std::numeric_limits<std::uint16_t>::max() },

  { BaseType::BASE_TYPE_INT, std::numeric_limits<std::int32_t>::min(), (std::uint64_t)std::numeric_limits<std::int32_t>::max() },
  { BaseType::BASE_TYPE_UINT, std::numeric_limits<std::uint32_t>::min(), std::numeric_limits<std::uint32_t>::max() },

  { BaseType::BASE_TYPE_LONG, std::numeric_limits<std::int64_t>::min(), (std::uint64_t)std::numeric_limits<std::int64_t>::max() },
  { BaseType::BASE_TYPE_ULONG, (std::int64_t)std::numeric_limits<std::uint64_t>::min(), std::numeric_limits<std::uint64_t>::max() },
};

struct DecimalInfo {
	BaseType type;
	int bits;
	const char* name() const {
		return kTypeNames[type];
	}
} DecimalInfos[] = {
  { BaseType::BASE_TYPE_FLOAT, 32 },
  { BaseType::BASE_TYPE_DOUBLE, 64 },
};

std::string GenType(const std::string &name, BaseType type) {
  for (auto p : IntegerInfos) {
    if (p.type == type) {
      return "\"$ref\" : \"#/definitions/" + std::string(p.name()) + "\"";
    }
  }
  for (auto p : DecimalInfos) {
	  if (p.type == type) {
		  return "\"$ref\" : \"#/definitions/" + std::string(p.name()) + "\"";
	  }
  }
  return "\"type\" : \"" + name + "\"";
}

std::string GenType(const std::string &name) {
  return "\"type\" : \"" + name + "\"";
}

std::string GenType(const Type &type) {
  if (type.enum_def != nullptr && !type.enum_def->is_union) {
    // it is a reference to an enum type
    if (type.base_type == BASE_TYPE_VECTOR) {
      std::string typeline;
      typeline.append("\"type\" : \"array\", \"items\" : { " + GenTypeRef(type.enum_def) + " }");
      return typeline;
    } else {
      return GenTypeRef(type.enum_def);
    }
  }
  switch (type.base_type) {
    case BASE_TYPE_VECTOR: {
      std::string typeline;
      typeline.append("\"type\" : \"array\", \"items\" : { ");
      if (type.element == BASE_TYPE_STRUCT) {
        typeline.append(GenTypeRef(type.struct_def));
      } else {
        typeline.append(GenType(GenNativeType(type.element), type.element));
      }
      typeline.append(" }");
      return typeline;
    }
    case BASE_TYPE_STRUCT: {
      return GenTypeRef(type.struct_def);
    }
    case BASE_TYPE_UNION: {
      return GenTypeRef(type.enum_def, "Union");

    }
    case BASE_TYPE_UTYPE: return GenTypeRef(type.enum_def);
    default: return GenType(GenNativeType(type.base_type), type.base_type);
  }
}

template<typename T>
static std::string ToString(T val)
{
  std::stringstream ss;
  ss << val;
  return ss.str();
}

class JsonSchemaGenerator : public BaseGenerator {
 private:
  CodeWriter code_;

 public:
  JsonSchemaGenerator(const Parser &parser, const std::string &path,
                      const std::string &file_name)
      : BaseGenerator(parser, path, file_name, "", "") {}

  explicit JsonSchemaGenerator(const BaseGenerator &base_generator)
      : BaseGenerator(base_generator) {}
  bool isExclusive(const std::string& path) const {
	  std::string refPath = "/" + file_name_ + ".fbs";
	  return path.find(refPath) != std::string::npos;
  }
  void writeBasicInfo(const Definition& def, const std::string& indent) {
	  code_ += indent + "\"exclusiveDefinition\" : " + std::string(isExclusive(def.file) ? "true" : "false") + ",";
	  if (!def.attributes.dict.empty()) {
		  code_ += indent + "\"attributes\" : { ";
		  std::string attributes;
		  for (auto attr = def.attributes.dict.begin(); attr != def.attributes.dict.end(); ++attr) {
			  std::string attribute = indent + "  \"" + attr->first + "\": \"" + attr->second->constant + "\"";
			  if (std::next(attr) != def.attributes.dict.end()) attribute += ", ";
			  code_ += attribute;
		  }
		  code_ += indent + "},";
	  }
	  code_ += indent + "\"namespace\" : \"" + def.GetFullyQualifiedNamespace() + "\",";
	  code_ += indent + "\"name\" : \"" + def.name + "\",";
	  std::string comment;
	  const auto &comment_lines = def.doc_comment;
	  for (auto comment_line = comment_lines.cbegin();
		  comment_line != comment_lines.cend(); ++comment_line) {
		  comment.append(*comment_line);
	  }
	  if (comment.size() > 0) {
		  code_ += "      \"description\" : \"" + comment + "\",";
	  }
  }

  bool generate() {
    code_.Clear();
    code_ += "{";
    code_ += "  \"$schema\": \"http://json-schema.org/draft-04/schema#\",";
    code_ += "  \"definitions\": {";
    for(auto prim : IntegerInfos) {
      code_ += "    \"" + std::string(prim.name()) + "\" : {";
      code_ += "      \"type\": \"integer\",";
      code_ += "      \"name\": \"" + std::string(prim.name()) + "\",";
      code_ += "      \"minimum\": " + ToString(prim.minValue) + ",";
      code_ += "      \"maximum\": " + ToString(prim.maxValue);
      code_ += "    },";  // close type
    }
	  for (auto prim : DecimalInfos) {
		  code_ += "    \"" + std::string(prim.name()) + "\" : {";
		  code_ += "      \"type\": \"number\",";
      code_ += "      \"name\": \"" + std::string(prim.name()) + "\",";
		  code_ += "      \"bits\": " + ToString(prim.bits);
		  code_ += "    },";  // close type
	  }
    for (auto e = parser_.enums_.vec.cbegin(); e != parser_.enums_.vec.cend();
         ++e) {
	    const auto& def = *e;
      code_ += "    \"" + GenFullName(*e) + "\" : {";
      code_ += "      " + GenType("string") + ",";
	    writeBasicInfo(*def, "      ");
      code_ += "      \"isEnum\" : \"true\",";
      std::string enumdef("      \"enum\": [");
      for (auto enum_value = (*e)->vals.vec.begin();
           enum_value != (*e)->vals.vec.end(); ++enum_value) {
        enumdef.append("\"" + (*enum_value)->name + "\"");
        if (*enum_value != (*e)->vals.vec.back()) { enumdef.append(", "); }
      }
      enumdef.append("],");
      code_ += enumdef;
      std::string enumvalues("      \"enum_values\": [");
      for (auto enum_value = (*e)->vals.vec.begin();
        enum_value != (*e)->vals.vec.end(); ++enum_value) {
        enumvalues.append(ToString((*enum_value)->value));
        if (*enum_value != (*e)->vals.vec.back()) { enumvalues.append(", "); }
      }
      enumvalues.append("]");
      code_ += enumvalues;
      code_ += "    },";  // close type

      if ((*e)->is_union) {
        auto& enum_def = **e;
        code_ += "    \"" + GenFullName(*e) + "Union" + "\" : {";
	      writeBasicInfo(enum_def, "      ");
        code_ += "      \"isUnion\" : \"true\",";
        code_ += "      \"anyOf\": [";
        const auto &union_types = enum_def.vals.vec;
        for (auto ut = union_types.cbegin(); ut < union_types.cend(); ++ut) {
          auto &union_type = *ut;
          if (union_type->union_type.base_type == BASE_TYPE_NONE) { continue; }
          if (union_type->union_type.base_type == BASE_TYPE_STRUCT) {
            std::string elem("        { " + GenTypeRef(union_type->union_type.struct_def) + " }");
            if (union_type != *enum_def.vals.vec.rbegin()) {
              elem += ",";
            }
            code_ += elem;
          }
        }
        code_ += "      ]";
        code_ += "    },";  // close type
      }
    }
    for (auto s = parser_.structs_.vec.cbegin();
          s != parser_.structs_.vec.cend(); ++s) {
      const auto &structure = *s;
      code_ += "    \"" + GenFullName(structure) + "\" : {";
      code_ += "      " + GenType("object") + ",";
	    writeBasicInfo(*structure, "      ");
      code_ += "      \"properties\" : {";

      const auto &properties = structure->fields.vec;
      for (auto prop = properties.cbegin(); prop != properties.cend(); ++prop) {
        const auto &property = *prop;
        std::string typeLine("        \"" + property->name + "\" : { " +
                              GenType(property->value.type) + " }");
        if (property != properties.back()) { typeLine.append(","); }
        code_ += typeLine;
      }
      code_ += "      },";  // close properties
      if (structure->has_key) {
        std::string keyLine("      \"key\" : \"" + structure->GetKeyField()->name + "\",");
        code_ += keyLine;
      }
	    if (structure->fixed) {
		    code_ += "      \"struct\" : true,";
      }
      else {
        code_ += "      \"table\" : true,";
      }
      std::vector<FieldDef *> requiredProperties;
      std::copy_if(properties.begin(), properties.end(),
                    back_inserter(requiredProperties),
                    [](FieldDef const *prop) { return prop->required; });
      if (requiredProperties.size() > 0) {
        std::string required_string("      \"required\" : [");
        for (auto req_prop = requiredProperties.cbegin();
              req_prop != requiredProperties.cend(); ++req_prop) {
          required_string.append("\"" + (*req_prop)->name + "\"");
          if (*req_prop != requiredProperties.back()) {
            required_string.append(", ");
          }
        }
        required_string.append("],");
        code_ += required_string;
      }
	    code_ += "      \"additionalProperties\" : false";
	  
      std::string closeType("    }");
      if (*s != parser_.structs_.vec.back()) { closeType.append(","); }
        code_ += closeType;  // close type
      }
    code_ += "  },";  // close definitions

    // mark root type
    code_ += "  \"$ref\" : \"#/definitions/" +
             GenFullName(parser_.root_struct_def_) + "\"";

    code_ += "}";  // close schema root
    const std::string file_path = GeneratedFileName(path_, file_name_);
    const std::string final_code = code_.ToString();
    return SaveFile(file_path.c_str(), final_code, false);
  }
};
}  // namespace jsons

bool GenerateJsonSchema(const Parser &parser, const std::string &path,
                        const std::string &file_name) {
  jsons::JsonSchemaGenerator generator(parser, path, file_name);
  return generator.generate();
}
}  // namespace flatbuffers
