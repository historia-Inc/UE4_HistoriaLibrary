#include "JsonSpawnPrivatePCH.h"
#include "JsonSpawnLibrary.h"


UJsonSpawnLibrary::UJsonSpawnLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


namespace
{
	FString GetPropertyType(UObject* Object, FName PropName)
	{
		if(NULL == Object)
		{
			return FString();
		}

		UProperty* Prop = Object->GetClass()->FindPropertyByName(PropName);
		if(NULL != Prop)
		{
			return Prop->GetCPPType();
		}
		return FString();
	}

	template<typename T>
	void ApplyProperyValue(UObject* Object, FName PropName, T PropValue)
	{
		if(NULL == Object)
		{
			return;
		}

		UProperty* Prop = Object->GetClass()->FindPropertyByName(PropName);
		if(NULL != Prop)
		{
			T* ValuePtr = Prop->ContainerPtrToValuePtr<T>(Object);
			if(ValuePtr)
			{
				*ValuePtr = PropValue;
			}
		}
	}

	bool ParseVectorValue(TSharedRef<TJsonReader<TCHAR>>& JsonReader, FVector& OutPropValue)
	{
		EJsonNotation Notation;
		for(int32 i = 0; i < 3; ++i)
		{
			if(!JsonReader->ReadNext(Notation))
			{
				return false;
			}
			if(Notation == EJsonNotation::Number)
			{
				switch(i)
				{
				case 0: OutPropValue.X = static_cast<float>(JsonReader->GetValueAsNumber()); break;
				case 1: OutPropValue.Y = static_cast<float>(JsonReader->GetValueAsNumber()); break;
				case 2: OutPropValue.Z = static_cast<float>(JsonReader->GetValueAsNumber()); break;
				}
			}
			else
			{
				return false;
			}
		}

		if(!JsonReader->ReadNext(Notation))
		{
			return false;
		}
		if(Notation != EJsonNotation::ArrayEnd)
		{
			return false;
		}

		return true;
	}

	bool ParseRotatorValue(TSharedRef<TJsonReader<TCHAR>>& JsonReader, FRotator& OutPropValue)
	{
		FVector DummyVector;
		if(!ParseVectorValue(JsonReader, DummyVector))
		{
			return false;
		}

		OutPropValue.Roll  = DummyVector.X;
		OutPropValue.Pitch = DummyVector.Y;
		OutPropValue.Yaw   = DummyVector.Z;
		return true;
	}
}

TArray<AActor*> UJsonSpawnLibrary::SpawnActorsFromJson(FString Json)
{
	TArray<AActor*> Result;

	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<>::Create(Json);

	EJsonNotation Notation;

	if(!(  (JsonReader->ReadNext(Notation) && (EJsonNotation::ObjectStart == Notation))
		&& (JsonReader->ReadNext(Notation) && (EJsonNotation::ArrayStart  == Notation))
		))
	{
		UE_LOG(LogJsonSpawn, Warning, TEXT("Invalid JSON Format"));
		return Result;
	}

	AActor* SpawnedActor = NULL;
	while(JsonReader->ReadNext(Notation))
	{
		switch(Notation)
		{
		case EJsonNotation::Boolean:
			{
				FName PropName(*JsonReader->GetIdentifier());
				FString PropType = GetPropertyType(SpawnedActor, PropName);
				if(0 == FString(TEXT("bool")).Compare(PropType))
				{
					bool PropValue = JsonReader->GetValueAsBoolean();
					ApplyProperyValue<bool>(SpawnedActor, PropName, PropValue);
				}
				else
				{
					UE_LOG(LogJsonSpawn, Warning, TEXT("Invalid Boolean Value [%s.%s]"), (SpawnedActor ? *SpawnedActor->GetClass()->GetName() : TEXT("None")), *PropName.ToString());
				}
			} break;
		case EJsonNotation::String:
			{
				FName PropName(*JsonReader->GetIdentifier());
				FString PropType = GetPropertyType(SpawnedActor, PropName);
				if(0 == FString(TEXT("FString")).Compare(PropType))
				{
					FString PropValue = JsonReader->GetValueAsString();
					ApplyProperyValue<FString>(SpawnedActor, PropName, PropValue);
				}
				else
				{
					if(FName(TEXT("ClassName")) == PropName)
					{
						const FString& TypeName = JsonReader->GetValueAsString();
						if(!TypeName.IsEmpty())
						{
							UClass* ClassPtr = FindObject<UClass>(ANY_PACKAGE, *TypeName);
							if(ClassPtr)
							{
								SpawnedActor = GWorld->SpawnActor(ClassPtr);
								if(NULL != SpawnedActor)
								{
									Result.Add(SpawnedActor);
								}
								else
								{
									UE_LOG(LogJsonSpawn, Warning, TEXT("Failed Spawn Actor [%s]"), *TypeName);
								}
							}
							else
							{
								UE_LOG(LogJsonSpawn, Warning, TEXT("Not Found Class [%s]"), *TypeName);
								SpawnedActor = NULL;
							}
						}
					}
					else
					{
						UE_LOG(LogJsonSpawn, Warning, TEXT("Invalid String Value [%s.%s]"), (SpawnedActor ? *SpawnedActor->GetClass()->GetName() : TEXT("None")), *PropName.ToString());
					}
				}
			} break;
		case EJsonNotation::Number:
			{
				FName PropName(*JsonReader->GetIdentifier());
				FString PropType = GetPropertyType(SpawnedActor, PropName);
				if(0 == FString(TEXT("int32")).Compare(PropType))
				{
					int32 PropValue = static_cast<int32>(JsonReader->GetValueAsNumber());
					ApplyProperyValue<int32>(SpawnedActor, PropName, PropValue);
				}
				else if(0 == FString(TEXT("float")).Compare(PropType))
				{
					float PropValue = static_cast<float>(JsonReader->GetValueAsNumber());
					ApplyProperyValue<float>(SpawnedActor, PropName, PropValue);
				}
				else
				{
					UE_LOG(LogJsonSpawn, Warning, TEXT("Invalid Number Value [%s.%s]"), (SpawnedActor ? *SpawnedActor->GetClass()->GetName() : TEXT("None")), *PropName.ToString());
				}
			} break;
		case EJsonNotation::ArrayStart:
			{
				FName PropName(*JsonReader->GetIdentifier());
				FString PropType = GetPropertyType(SpawnedActor, PropName);
				if(0 == FString(TEXT("FVector")).Compare(PropType))
				{
					FVector PropValue;
					if(ParseVectorValue(JsonReader, PropValue))
					{
						ApplyProperyValue<FVector>(SpawnedActor, PropName, PropValue);
					}
					else
					{
						UE_LOG(LogJsonSpawn, Warning, TEXT("Invalid Vector Value [%s.%s]"), (SpawnedActor ? *SpawnedActor->GetClass()->GetName() : TEXT("None")), *PropName.ToString());
					}
				}
				else if(0 == FString(TEXT("FRotator")).Compare(PropType))
				{
					FRotator PropValue;
					if(ParseRotatorValue(JsonReader, PropValue))
					{
						ApplyProperyValue<FRotator>(SpawnedActor, PropName, PropValue);
					}
					else
					{
						UE_LOG(LogJsonSpawn, Warning, TEXT("Invalid Rotator Value [%s.%s]"), (SpawnedActor ? *SpawnedActor->GetClass()->GetName() : TEXT("None")), *PropName.ToString());
					}
				}
				else
				{
					if(FName(TEXT("Location")) == PropName)
					{
						FVector PropValue;
						if(ParseVectorValue(JsonReader, PropValue))
						{
							if(SpawnedActor)
							{
								SpawnedActor->SetActorLocation(PropValue);
							}
							else
							{
								UE_LOG(LogJsonSpawn, Warning, TEXT("Invalid Location. Actor hasn't spawned."));
							}
						}
						else
						{
							UE_LOG(LogJsonSpawn, Warning, TEXT("Invalid Location Value [%s]"), (SpawnedActor ? *SpawnedActor->GetClass()->GetName() : TEXT("None")));
						}
					}
					else if(FName(TEXT("Rotation")) == PropName)
					{
						FRotator PropValue;
						if(ParseRotatorValue(JsonReader, PropValue))
						{
							if(SpawnedActor)
							{
								SpawnedActor->SetActorRotation(PropValue);
							}
							else
							{
								UE_LOG(LogJsonSpawn, Warning, TEXT("Invalid Rotation. Actor hasn't spawned."));
							}
						}
						else
						{
							UE_LOG(LogJsonSpawn, Warning, TEXT("Invalid Rotaion Value [%s.%s]"), (SpawnedActor ? *SpawnedActor->GetClass()->GetName() : TEXT("None")), *PropName.ToString());
						}
					}
					else if(FName(TEXT("Scale")) == PropName)
					{
						FVector PropValue;
						if(ParseVectorValue(JsonReader, PropValue))
						{
							if(SpawnedActor)
							{
								SpawnedActor->SetActorScale3D(PropValue);
							}
							else
							{
								UE_LOG(LogJsonSpawn, Warning, TEXT("Invalid Scale. Actor hasn't spawned."));
							}
						}
						else
						{
							UE_LOG(LogJsonSpawn, Warning, TEXT("Invalid Scale Value [%s]"), (SpawnedActor ? *SpawnedActor->GetClass()->GetName() : TEXT("None")));
						}
					}
					else
					{
						UE_LOG(LogJsonSpawn, Warning, TEXT("Invalid Array Value [%s.%s]"), (SpawnedActor ? *SpawnedActor->GetClass()->GetName() : TEXT("None")), *PropName.ToString());
					}
				}
			} break;
		case EJsonNotation::ObjectStart:
		case EJsonNotation::ObjectEnd:
		case EJsonNotation::ArrayEnd:
		case EJsonNotation::Null:
		case EJsonNotation::Error:
			{
			} break;
		}
	}

	return Result;
}

