#pragma once
#include <cstdint>
#include <sstream>
#include <string>
#include <Unreal/Common.hpp>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>

#include <File/File.hpp>
#include <Unreal/NameTypes.hpp>

namespace RC::USMapWriter
{
    void static MappingsDump()

class Dumper : public IDumper
{
public:

    struct FPropertyData
    {
        Engine::FProperty* Prop;
        uint16_t Index;
        uint8_t ArrayDim;
        Engine::FName Name;
        EPropertyType Type;

        FPropertyData(Engine::FProperty* P, int Idx) :
            Prop(P),
            Index(Idx),
            ArrayDim(P->GetArrayDim()),
            Name(P->GetName()),
            Type(P->GetPropertyType())
        {
        }
    };
    
    void Run() const override;
};
    
class IBufferWriter
{
public:

    virtual FORCEINLINE void WriteString(std::string String) = 0;
    virtual FORCEINLINE void WriteString(std::string_view String) = 0;
    virtual FORCEINLINE void Write(void* Input, size_t Size) = 0;
    virtual FORCEINLINE void Seek(int Pos, int Origin = SEEK_CUR) = 0;
    virtual uint32_t Size() = 0;
};

class StreamWriter : IBufferWriter
{
    std::stringstream m_Stream;

public:

    ~StreamWriter()
    {
        m_Stream.flush();
    }

    FORCEINLINE std::stringstream& GetBuffer()
    {
        return m_Stream;
    }

    FORCEINLINE void WriteString(std::string String) override
    {
        m_Stream.write(String.c_str(), String.size());
    }

    FORCEINLINE void WriteString(std::string_view String) override
    {
        m_Stream.write(String.data(), String.size());
    }

    FORCEINLINE void Write(void* Input, size_t Size) override
    {
        m_Stream.write((char*)Input, Size);
    }

    FORCEINLINE void Seek(int Pos, int Origin = SEEK_CUR) override
    {
        m_Stream.seekp(Pos, Origin);
    }

    uint32_t Size() override
    {
        auto pos = m_Stream.tellp();
        this->Seek(0, SEEK_END);
        auto ret = m_Stream.tellp();
        this->Seek(pos, SEEK_SET);

        return ret;
    }

    template <typename T>
    FORCEINLINE void Write(T Input)
    {
        Write(&Input, sizeof(T));
    }
};

class FileWriter : IBufferWriter
{
    FILE* m_File;

public:

    FileWriter(const char* FileName)
    {
        fopen_s(&m_File, FileName, "wb");
    }

    ~FileWriter()
    {
        std::fclose(m_File);
    }

    FORCEINLINE void WriteString(std::string String) override
    {
        std::fwrite(String.c_str(), String.length(), 1, m_File);
    }

    FORCEINLINE void WriteString(std::string_view String) override
    {
        std::fwrite(String.data(), String.size(), 1, m_File);
    }

    FORCEINLINE void Write(void* Input, size_t Size) override
    {
        std::fwrite(Input, Size, 1, m_File);
    }

    FORCEINLINE void Seek(int Pos, int Origin = SEEK_CUR) override
    {
        std::fseek(m_File, Pos, Origin);
    }

    uint32_t Size() override
    {
        auto pos = std::ftell(m_File);
        std::fseek(m_File, 0, SEEK_END);
        auto ret = std::ftell(m_File);
        std::fseek(m_File, pos, SEEK_SET);
        return ret;
    }

    template <typename T>
    FORCEINLINE void Write(T Input)
    {
        Write(&Input, sizeof(T));
    }
};

    enum class ECompressionMethod : uint8_t
    {
        None,
        Oodle,
        Brotli,
        Unknown = 0xFF
    };

    enum class EPropertyType : uint8_t
    {
        ByteProperty,
        BoolProperty,
        IntProperty,
        FloatProperty,
        ObjectProperty,
        NameProperty,
        DelegateProperty,
        DoubleProperty,
        ArrayProperty,
        StructProperty,
        StrProperty,
        TextProperty,
        InterfaceProperty,
        MulticastDelegateProperty,
        WeakObjectProperty,
        LazyObjectProperty,
        AssetObjectProperty,
        SoftObjectProperty,
        UInt64Property,
        UInt32Property,
        UInt16Property,
        Int64Property,
        Int16Property,
        Int8Property,
        MapProperty,
        SetProperty,
        EnumProperty,
        FieldPathProperty,
        EnumAsByteProperty,

        Unknown = 0xFF
    };

    enum class EEnumFlags
    {
        None,
        Flags = 0x00000001,
        NewerVersionExists = 0x00000002,
    };


template <typename T>
EPropertyType FProperty::GetPropertyType() 
{
	switch (this->GetClass()->GetId())
	{
		case CASTCLASS_FObjectProperty:
		case CASTCLASS_FClassProperty:
		case CASTCLASS_FObjectPtrProperty:
		case CASTCLASS_FClassPtrProperty:
		{
			return EPropertyType::ObjectProperty;
		}
		case CASTCLASS_FStructProperty:
		{
			return EPropertyType::StructProperty;
		}
		case CASTCLASS_FInt8Property:
		{
			return EPropertyType::Int8Property;
		}
		case CASTCLASS_FInt16Property:
		{
			return EPropertyType::Int16Property;
		}
		case CASTCLASS_FIntProperty:
		{
			return EPropertyType::IntProperty;
		}
		case CASTCLASS_FInt64Property:
		{
			return EPropertyType::Int16Property;
		}
		case CASTCLASS_FUInt16Property:
		{
			return EPropertyType::UInt16Property;
		}
		case CASTCLASS_FUInt32Property:
		{
			return EPropertyType::UInt32Property;
		}
		case CASTCLASS_FUInt64Property:
		{
			return EPropertyType::UInt64Property;
		}
		case CASTCLASS_FArrayProperty:
		{
			return EPropertyType::ArrayProperty;
		}
		case CASTCLASS_FFloatProperty:
		{
			return EPropertyType::FloatProperty;
		}
		case CASTCLASS_FDoubleProperty:
		{
			return EPropertyType::DoubleProperty;
		}
		case CASTCLASS_FBoolProperty:
		{
			return EPropertyType::BoolProperty;
		}
		case CASTCLASS_FStrProperty:
		{
			return EPropertyType::StrProperty;
		}
		case CASTCLASS_FNameProperty:
		{
			return EPropertyType::NameProperty;
		}
		case CASTCLASS_FTextProperty:
		{
			return EPropertyType::TextProperty;
		}
		case CASTCLASS_FEnumProperty:
		{
			return EPropertyType::EnumProperty;
		}
		case CASTCLASS_FInterfaceProperty:
		{
			return EPropertyType::InterfaceProperty;
		}
		case CASTCLASS_FMapProperty:
		{
			return EPropertyType::MapProperty;
		}
		case CASTCLASS_FByteProperty:
		{
			FByteProperty* ByteProp = static_cast<FByteProperty*>(this);

			if (ByteProp->GetEnum())
				return EPropertyType::EnumAsByteProperty;
			
			return EPropertyType::ByteProperty;
		}
		case CASTCLASS_FMulticastSparseDelegateProperty:
		{
			return EPropertyType::MulticastDelegateProperty;
		}
		case CASTCLASS_FDelegateProperty:
		{
			return EPropertyType::DelegateProperty;
		}
		case CASTCLASS_FSoftObjectProperty:
		case CASTCLASS_FSoftClassProperty:
		case CASTCLASS_FWeakObjectProperty:
		{
			return EPropertyType::SoftObjectProperty;
		}
		case CASTCLASS_FLazyObjectProperty:
		{
			return EPropertyType::LazyObjectProperty;
		}
		case CASTCLASS_FSetProperty:
		{
			return EPropertyType::SetProperty;
		}
		default:
		{
			return EPropertyType::Unknown;
		}
	}
}
    
}