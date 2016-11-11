/*
Copyright 2016 Krzysztof Lis

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "AugmentedUnreality.h"
#include "Engine.h"
#include "AURVideoScreenBase.h"

UAURVideoScreenBase::UAURVideoScreenBase()
	: UseGlobalDriver(true)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	this->bTickInEditor = false;
	this->bAutoRegister = true;
	this->bAutoActivate = true;

	this->SetEnableGravity(false);
	this->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	this->bGenerateOverlapEvents = false;
}

void UAURVideoScreenBase::UseDriver(UAURDriver* new_driver)
{
	if (VideoDriver)
	{
		VideoDriver->OnCameraParametersChange.RemoveAll(this);
	}

	VideoDriver = new_driver;

	if (VideoDriver)
	{
		// Switch to this driver's texture
		OnCameraPropertiesChange(VideoDriver);

		// Subscribe to future changes
		VideoDriver->OnCameraParametersChange.AddUniqueDynamic(this, &UAURVideoScreenBase::OnCameraPropertiesChange);
	}
}

void UAURVideoScreenBase::OnCameraPropertiesChange(UAURDriver* Driver)
{
	if (VideoDriver && this->ScreenMaterial)
	{
		this->ScreenMaterial->SetTextureParameterValue(FName("VideoTexture"), VideoDriver->GetOutputTexture());
	}
}

void UAURVideoScreenBase::BeginPlay()
{
	Super::BeginPlay();

	this->ScreenMaterial = FindScreenMaterial();

	if (UseGlobalDriver)
	{
		// Try getting current driver
		if (UAURDriver::GetCurrentDriver())
		{
			UseDriver(UAURDriver::GetCurrentDriver());
		}

		// Subscribe for know when a driver is created later
		UAURDriver::GetDriverInstanceChangeDelegate().AddUniqueDynamic(this, &UAURVideoScreenBase::UseDriver);
	}
}

void UAURVideoScreenBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UAURDriver::UnbindOnDriverInstanceChange(this);

	Super::EndPlay(EndPlayReason);
}

UMaterialInstanceDynamic* UAURVideoScreenBase::FindScreenMaterial()
{
	// Iterate over materials to find the reference to the dynamic texture
	// so that its content can be written to later.

	for (int32 material_idx = 0; material_idx < this->GetNumMaterials(); material_idx++)
	{
		UMaterialInterface* material = this->GetMaterial(material_idx);
		UTexture* texture_param_value = nullptr;
		if (material->GetTextureParameterValue("VideoTexture", texture_param_value))
		{
			UMaterialInstanceDynamic* dynamic_material_instance = Cast<UMaterialInstanceDynamic>(material);
			if (!dynamic_material_instance)
			{
				dynamic_material_instance = UMaterialInstanceDynamic::Create(material, this);
				this->SetMaterial(material_idx, dynamic_material_instance);
			}

			return dynamic_material_instance;
		}
	}
	
	return nullptr;
}
