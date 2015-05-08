#include "JsonSpawnPrivatePCH.h"
#include "JsonSpawnModule.h"

#include "Serialization/JsonTypes.h"


DEFINE_LOG_CATEGORY(LogJsonSpawn);

class FJsonSpawnModule : public IJsonSpawnModule
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
IMPLEMENT_MODULE(FJsonSpawnModule, JsonSpawnModule)


void FJsonSpawnModule::StartupModule()
{
}
void FJsonSpawnModule::ShutdownModule()
{
}
