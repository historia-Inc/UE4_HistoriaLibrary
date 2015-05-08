#include "JsonPropPrivatePCH.h"
#include "JsonPropModule.h"

#include "Factories/Factory.h"
#include "ContentBrowserModule.h"
#include "Serialization/JsonTypes.h"
#include "DesktopPlatformModule.h"
#include "IMainFrameModule.h"
#include "EditorDirectories.h"


DEFINE_LOG_CATEGORY(LogJsonProp);

#define LOCTEXT_NAMESPACE "JsonPropModule"

FDelegateHandle SelectedAssetsDelegateHandle;

class FJsonPropModule : public IJsonPropModule
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedRef<FExtender> GetExtender(const TArray<FAssetData>& SelectedAssets);
	void OnWindowMenuExtension(FMenuBuilder& MenuBuilder);
	void OnImportJson();

private:
	TSharedPtr<FExtender> Extender;
	FContentBrowserMenuExtender_SelectedAssets SelectedAssetsDelegate;

	TArray<FAssetData> SelectedAssets;
};

IMPLEMENT_MODULE(FJsonPropModule, JsonPropModule)



void FJsonPropModule::StartupModule()
{
	if(IsRunningCommandlet()) { return ; }

	Extender = MakeShareable(new FExtender);
	Extender->AddMenuExtension(
		"GetAssetActions",
		EExtensionHook::After,
		NULL,
		FMenuExtensionDelegate::CreateRaw(this, &FJsonPropModule::OnWindowMenuExtension)
		);
	SelectedAssetsDelegate.BindRaw(this, &FJsonPropModule::GetExtender);

	FContentBrowserModule& ContentBrowser = 
		FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowser.GetAllAssetViewContextMenuExtenders().Add(SelectedAssetsDelegate);
	SelectedAssetsDelegateHandle = SelectedAssetsDelegate.GetHandle();
}

void FJsonPropModule::ShutdownModule()
{
	FContentBrowserModule& ContentBrowser = 
		FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowser.GetAllAssetViewContextMenuExtenders().RemoveAll(
		[](const FContentBrowserMenuExtender_SelectedAssets& Delegate)
		{
			return Delegate.GetHandle() == SelectedAssetsDelegateHandle;
	}
		);
}

TSharedRef<FExtender> FJsonPropModule::GetExtender(const TArray<FAssetData>& InSelectedAssets)
{
	SelectedAssets = InSelectedAssets;
	return Extender.ToSharedRef();
}

void FJsonPropModule::OnWindowMenuExtension(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ImportJsonMenuTitle", "Import from JSON"),
		LOCTEXT("ImportJsonMenuToolTip", "Import Properties from JSON."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FJsonPropModule::OnImportJson)));
}


namespace
{
	FString GetPropertyType(FAssetData& SelectedAsset, FName PropName)
	{
		UObject* Asset = SelectedAsset.GetAsset();
		if(NULL == Asset)
		{
			return FString();
		}

		UProperty* Prop = NULL;
		if(UBlueprint* BPAsset = Cast<UBlueprint>(Asset))
		{
			Prop = BPAsset->GeneratedClass->FindPropertyByName(PropName);
		}
		else
		{
			Prop = Asset->GetClass()->FindPropertyByName(PropName);
		}
		if(NULL != Prop)
		{
			return Prop->GetCPPType();
		}
		return FString();
	}

	template<typename T>
	void ApplyProperyValue(FAssetData& SelectedAsset, FName PropName, T PropValue)
	{
		UObject* Asset = SelectedAsset.GetAsset();
		if(NULL == Asset)
		{
			return;
		}

		UObject* DefaultObject = NULL;
		UProperty* Prop = NULL;

		if(UBlueprint* BPAsset = Cast<UBlueprint>(Asset))
		{
			DefaultObject = BPAsset->GeneratedClass->GetDefaultObject();
			Prop = BPAsset->GeneratedClass->FindPropertyByName(PropName);
		}
		else
		{
			DefaultObject = Asset->GetClass()->GetDefaultObject();
			Prop = Asset->GetClass()->FindPropertyByName(PropName);
		}

		if((NULL != DefaultObject) && (NULL != Prop))
		{
			T* ValuePtr = Prop->ContainerPtrToValuePtr<T>(DefaultObject);
			if(ValuePtr)
			{
				*ValuePtr = PropValue;
				DefaultObject->MarkPackageDirty();
			}
		}
	}
}

void FJsonPropModule::OnImportJson()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	bool bOpened = false;
	if(!DesktopPlatform)
	{
		return;
	}

	void* ParentWindowWindowHandle = NULL;
	IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
	const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
	if ( MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid() )
	{
		ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
	}

	TArray<FString> OpenFileNames;
	bOpened = DesktopPlatform->OpenFileDialog(
		ParentWindowWindowHandle,
		TEXT("Import Json"),
		FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_IMPORT),
		TEXT(""),
		TEXT("Json (*.json)|*.json"),
		EFileDialogFlags::None,
		OpenFileNames
	);
	if(0 == OpenFileNames.Num())
	{
		return;
	}
	
	FString Json;
	if(!FFileHelper::LoadFileToString(Json, *OpenFileNames[0]))
	{
		return;
	}

	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<>::Create(Json);
	
	EJsonNotation Notation;

	if(!(  (JsonReader->ReadNext(Notation) && (EJsonNotation::ObjectStart == Notation))
		&& (JsonReader->ReadNext(Notation) && (EJsonNotation::ArrayStart  == Notation))
		))
	{
		UE_LOG(LogJsonProp, Warning, TEXT("Invalid JSON Format"));
		return ;
	}

	FName AssetName;
	while(JsonReader->ReadNext(Notation))
	{
		switch(Notation)
		{
		case EJsonNotation::Boolean:
			{
				FName PropName(*JsonReader->GetIdentifier());
				for(int32 i = 0; i < SelectedAssets.Num(); ++i)
				{
					if(AssetName == SelectedAssets[i].ObjectPath)
					{
						FString PropType = GetPropertyType(SelectedAssets[i], PropName);
						if(0 == FString(TEXT("bool")).Compare(PropType))
						{
							bool PropValue = JsonReader->GetValueAsBoolean();
							ApplyProperyValue<bool>(SelectedAssets[i], PropName, PropValue);
						}
					}
				}
			} break;
		case EJsonNotation::String:
			{
				FName PropName(*JsonReader->GetIdentifier());
				if(FName(TEXT("ClassName")) == PropName)
				{
					AssetName = FName(*JsonReader->GetValueAsString());
				}
				else
				{
					for(int32 i = 0; i < SelectedAssets.Num(); ++i)
					{
						if(AssetName == SelectedAssets[i].ObjectPath)
						{
							FString PropType = GetPropertyType(SelectedAssets[i], PropName);
							if(0 == FString(TEXT("FString")).Compare(PropType))
							{
								FString PropValue = JsonReader->GetValueAsString();
								ApplyProperyValue<FString>(SelectedAssets[i], PropName, PropValue);
							}
						}
					}
				}
			} break;
		case EJsonNotation::Number:
			{
				FName PropName(*JsonReader->GetIdentifier());
				for(int32 i = 0; i < SelectedAssets.Num(); ++i)
				{
					if(AssetName == SelectedAssets[i].ObjectPath)
					{
						FString PropType = GetPropertyType(SelectedAssets[i], PropName);
						if(0 == FString(TEXT("int32")).Compare(PropType))
						{
							int32 PropValue = static_cast<int32>(JsonReader->GetValueAsNumber());
							ApplyProperyValue<int32>(SelectedAssets[i], PropName, PropValue);
						}
						else if(0 == FString(TEXT("float")).Compare(PropType))
						{
							float PropValue = static_cast<float>(JsonReader->GetValueAsNumber());
							ApplyProperyValue<float>(SelectedAssets[i], PropName, PropValue);
						}
					}
				}
			} break;
		case EJsonNotation::ArrayStart:
			{
				FName PropName(*JsonReader->GetIdentifier());
				for(int32 i = 0; i < SelectedAssets.Num(); ++i)
				{
					if(AssetName == SelectedAssets[i].ObjectPath)
					{
						FString PropType = GetPropertyType(SelectedAssets[i], PropName);
						if(0 == FString(TEXT("FVector")).Compare(PropType))
						{
							FVector PropValue;
							for(int32 ii = 0; ii < 3; ++ii)
							{
								if(!JsonReader->ReadNext(Notation))
								{
									break;
								}
								if(Notation == EJsonNotation::Number)
								{
									switch(ii)
									{
										case 0: PropValue.X = static_cast<float>(JsonReader->GetValueAsNumber()); break;
										case 1: PropValue.Y = static_cast<float>(JsonReader->GetValueAsNumber()); break;
										case 2: PropValue.Z = static_cast<float>(JsonReader->GetValueAsNumber()); break;
									}
								}
							}
							ApplyProperyValue<FVector>(SelectedAssets[i], PropName, PropValue);
						}
						else if(0 == FString(TEXT("FRotator")).Compare(PropType))
						{
							FRotator PropValue;
							for(int32 ii = 0; ii < 3; ++ii)
							{
								if(!JsonReader->ReadNext(Notation))
								{
									break;
								}
								if(Notation == EJsonNotation::Number)
								{
									switch(ii)
									{
										case 0: PropValue.Roll  = static_cast<float>(JsonReader->GetValueAsNumber()); break;
										case 1: PropValue.Pitch = static_cast<float>(JsonReader->GetValueAsNumber()); break;
										case 2: PropValue.Yaw   = static_cast<float>(JsonReader->GetValueAsNumber()); break;
									}
								}
							}
							ApplyProperyValue<FRotator>(SelectedAssets[i], PropName, PropValue);
						}
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
}

#undef LOCTEXT_NAMESPACE