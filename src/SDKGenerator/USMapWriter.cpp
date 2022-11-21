#include <format>
#include <cwctype>

#include <SDKGenerator/USMapWriter.hpp>
#include <SDKGenerator/Common.hpp>
#include <UE4SSProgram.hpp>
#include "parallel_hashmap/phmap.h"
#include <codecvt>
#pragma warning(disable: 4005)
#include <Unreal/TypeChecker.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/FProperty.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UInterface.hpp>
#include <Unreal/UScriptStruct.hpp>
#include <Unreal/UEnum.hpp>
#include <Unreal/Property/FClassProperty.hpp>
#include <Unreal/Property/FStructProperty.hpp>
#include <Unreal/Property/FArrayProperty.hpp>
#include <Unreal/Property/FMapProperty.hpp>
#include <Unreal/Property/FDelegateProperty.hpp>
#include <Unreal/Property/FMulticastInlineDelegateProperty.hpp>
#include <Unreal/Property/FMulticastSparseDelegateProperty.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Unreal/Property/FEnumProperty.hpp>
#include <Unreal/Property/NumericPropertyTypes.hpp>
#pragma warning(default: 4005)

namespace RC::USMapWriter
{
    using TypeChecker = RC::Unreal::TypeChecker;
    namespace UObjectGlobals = RC::Unreal::UObjectGlobals;
    using EObjectFlags = RC::Unreal::EObjectFlags;
    using FName = RC::Unreal::FName;
    using UObject = RC::Unreal::UObject;
    using UClass = RC::Unreal::UClass;
    using UInterface = RC::Unreal::UInterface;
    using UScriptStruct = RC::Unreal::UScriptStruct;
    using UFunction = RC::Unreal::UFunction;
    using XProperty = RC::Unreal::FProperty;
    using UStruct = RC::Unreal::UStruct;
    using UEnum = Unreal::UEnum;
    using FObjectProperty = RC::Unreal::FObjectProperty;
    using FClassProperty = RC::Unreal::FClassProperty;
    using FEnumProperty = RC::Unreal::FEnumProperty;
    using FStructProperty = RC::Unreal::FStructProperty;
    using FArrayProperty = RC::Unreal::FArrayProperty;
    using FMapProperty = RC::Unreal::FMapProperty;
    using FByteProperty = RC::Unreal::FByteProperty;
    using FDelegateProperty = RC::Unreal::FDelegateProperty;
    using FMulticastInlineDelegateProperty = RC::Unreal::FMulticastInlineDelegateProperty;
    using FMulticastSparseDelegateProperty = RC::Unreal::FMulticastSparseDelegateProperty;
    using FPropertyData = Dumper::FPropertyData;

   void MappingsDump()
    {
       
    auto CompressionMethod = ECompressionMethod::Oodle;
    StreamWriter Buffer;
	phmap::parallel_flat_hash_map<FName, int> NameMap;

	std::vector<class UEnum*> Enums;
	std::vector<class UStruct*> Structs; // TODO: a better way than making this completely dynamic

	std::function<void(class FProperty*&, EPropertyType)> WritePropertyWrapper{}; // hacky.. i know

	auto WriteProperty = [&](FProperty*& Prop, EPropertyType Type)
	{
		if (Type == EPropertyType::EnumAsByteProperty)
			Buffer.Write(EPropertyType::EnumProperty);
		else Buffer.Write(Type);

		switch (Type)
		{
			case EPropertyType::EnumProperty:
			{
				auto EnumProp = static_cast<FEnumProperty*>(Prop);

				auto Inner = EnumProp->GetUnderlyingProperty();
				auto InnerType = Inner->GetPropertyType();
				WritePropertyWrapper(Inner, InnerType);
			    auto EnumName = EnumProp->GetFName();
				Buffer.Write(NameMap[EnumName]);

				break;
			}
			case EPropertyType::EnumAsByteProperty:
			{
				Buffer.Write(EPropertyType::ByteProperty);
				Buffer.Write(NameMap[static_cast<FByteProperty*>(Prop)->GetFName()]);

				break;
			}
			case EPropertyType::StructProperty:
			{
				Buffer.Write(NameMap[static_cast<FStructProperty*>(Prop)->GetFName()]);
				break;
			}
			case EPropertyType::SetProperty:
			case EPropertyType::ArrayProperty:
			{
				auto Inner = static_cast<FArrayProperty*>(Prop)->GetInner();
				auto InnerType = Inner->GetPropertyType();
				WritePropertyWrapper(Inner, InnerType);

				break;
			}
			case EPropertyType::MapProperty:
			{
				auto Inner = static_cast<FMapProperty*>(Prop)->GetKeyProp();
				auto InnerType = Inner->GetPropertyType();
				WritePropertyWrapper(Inner, InnerType);

				auto Value = static_cast<FMapProperty*>(Prop)->GetValueProp();
				auto ValueType = Value->GetPropertyType();
				WritePropertyWrapper(Value, ValueType);

				break;
			}
		}
	};

	WritePropertyWrapper = WriteProperty;

	UObjectGlobals::ForEachUObject([&](Unreal::UObject*& CurObject)
		{
	    
	        UClass* meta_class = CurObject->GetClassPrivate();
	        UObject* typed_object = static_cast<UObject*>(CurObject);
			if (meta_class == UClass::StaticClass() ||
				meta_class == UScriptStruct::StaticClass())
			{
				auto Struct = static_cast<UStruct*>(CurObject);

				Structs.push_back(Struct);
				
				NameMap.insert_or_assign(Struct->GetFName(), 0);

				if (Struct->GetSuperStruct() && !NameMap.contains(Struct->GetSuperStruct()->GetFName()))
					NameMap.insert_or_assign(Struct->GetSuperStruct()->GetFName(), 0);

				auto Props = Struct->GetChildProperties();

				while (Props)
				{
					NameMap.insert_or_assign(Props->GetFName(), 0);
					Props = static_cast<FProperty*>(Props->GetNext());
				}
			}
			else if (meta_class->IsChildOf<UEnum>)
			{
				UEnum* CurEnum = static_cast<UEnum*>(typed_object);
				Enums.push_back(CurEnum);
				
				NameMap.insert_or_assign(CurEnum->GetFName(), 0);

				for (auto i = 0; i < CurEnum->Names.Num(); i++)
				{
					NameMap.insert_or_assign(CurEnum->Names[i].Key, 0);
				}
			}
		});

	Buffer.Write<int>(NameMap.size());

	int CurrentNameIndex = 0;

	    std::wstring string_to_convert;

	    //setup converter
	    using convert_type = std::codecvt_utf8<wchar_t>;
	    std::wstring_convert<convert_type, wchar_t> converter;
	    
	for (auto N : NameMap)
	{
		NameMap[N.first] = CurrentNameIndex;

		auto Name = N.first.ToString();
	    std::string Converted_Name = converter.to_bytes(Name);
		std::string_view NameView = Converted_Name;

		auto Find = Name.find("::");
		if (Find != std::string::npos)
		{
			NameView = NameView.substr(Find + 2);
		}

		Buffer.Write<uint8_t>(NameView.length());
		Buffer.WriteString(NameView);

		CurrentNameIndex++;
	}

	Buffer.Write<uint32_t>(Enums.size());

	for (auto Enum : Enums)
	{
		Buffer.Write(NameMap[Enum->GetName()]);
		Buffer.Write<uint8_t>(Enum->Names.Num());

		for (size_t i = 0; i < Enum->Names.Num(); i++)
		{
			Buffer.Write<int>(NameMap[Enum->Names[i].Key]);
		}
	}

	Buffer.Write<uint32_t>(Structs.size());

	for (auto Struct : Structs)
	{
		Buffer.Write(NameMap[Struct->GetFName()]);
		Buffer.Write<int32_t>(Struct->GetSuperStruct() ? NameMap[Struct->GetSuperStruct()->GetFName()] : 0xffffffff);

		std::vector<FPropertyData> Properties;

		auto Props = Struct->GetChildProperties();
		uint16_t PropCount = 0;
		uint16_t SerializablePropCount = 0;

		while (Props)
		{
			FPropertyData Data(Props, PropCount);

			Properties.push_back(Data);
			Props = static_cast<FProperty*>(Props->GetNext());

			PropCount += Data.ArrayDim;
			SerializablePropCount++;
		}

		Buffer.Write(PropCount);
		Buffer.Write(SerializablePropCount);

		for (auto P : Properties)
		{
			Buffer.Write<uint16_t>(P.Index);
			Buffer.Write(P.ArrayDim);
			Buffer.Write(NameMap[P.Name]);

			WriteProperty(P.Prop, P.Type);
		}
	}

	std::vector<uint8_t> UsmapData;

	switch (CompressionMethod) 
	{
		default:
		{
			std::string UncompressedStream = Buffer.GetBuffer().str();
			UsmapData.resize(UncompressedStream.size());
			memcpy(UsmapData.data(), UncompressedStream.data(), UsmapData.size());
		}
	}

	auto FileOutput = FileWriter("Mappings.usmap");

	FileOutput.Write<uint16_t>(0x30C4); //magic
	FileOutput.Write<uint8_t>(0); //version
	FileOutput.Write(CompressionMethod); //compression
	FileOutput.Write<uint32_t>(UsmapData.size()); //compressed size
	FileOutput.Write<uint32_t>(Buffer.Size()); //decompressed size

	FileOutput.Write(UsmapData.data(), UsmapData.size());
	}
    
    
}