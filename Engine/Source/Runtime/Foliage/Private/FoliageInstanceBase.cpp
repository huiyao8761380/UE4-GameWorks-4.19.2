// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#include "FoliagePrivate.h"
#include "FoliageInstanceBase.h"
#include "InstancedFoliage.h"
#include "Engine/WorldComposition.h"

#if WITH_EDITORONLY_DATA

FFoliageInstanceBaseInfo::FFoliageInstanceBaseInfo()
	: CachedLocation(FVector::ZeroVector)
	, CachedRotation(FRotator::ZeroRotator)
	, CachedDrawScale(1.0f, 1.0f, 1.0f)
{}

FFoliageInstanceBaseInfo::FFoliageInstanceBaseInfo(UActorComponent* InComponent)
	: BasePtr(InComponent)
	, CachedLocation(FVector::ZeroVector)
	, CachedRotation(FRotator::ZeroRotator)
	, CachedDrawScale(1.0f, 1.0f, 1.0f)
{
	UpdateLocationFromComponent(InComponent);
}

void FFoliageInstanceBaseInfo::UpdateLocationFromComponent(UActorComponent* InComponent)
{
	if (InComponent)
	{
		AActor* Owner = Cast<AActor>(InComponent->GetOuter());
		if (Owner)
		{
			const USceneComponent* RootComponent = Owner->GetRootComponent();
			if (RootComponent)
			{
				CachedLocation = RootComponent->RelativeLocation;
				CachedRotation = RootComponent->RelativeRotation;
				CachedDrawScale = RootComponent->RelativeScale3D;
			}
		}
	}
}

FArchive& operator << (FArchive& Ar, FFoliageInstanceBaseInfo& BaseInfo)
{
	Ar << BaseInfo.BasePtr;
	Ar << BaseInfo.CachedLocation;
	Ar << BaseInfo.CachedRotation;
	Ar << BaseInfo.CachedDrawScale;

	return Ar;
}

//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////
FFoliageInstanceBaseId FFoliageInstanceBaseCache::InvalidBaseId = INDEX_NONE;

FFoliageInstanceBaseCache::FFoliageInstanceBaseCache()
	: NextBaseId(1)
{
}

FArchive& operator << (FArchive& Ar, FFoliageInstanceBaseCache& InstanceBaseCache)
{
	Ar << InstanceBaseCache.NextBaseId;
	Ar << InstanceBaseCache.InstanceBaseMap;
	Ar << InstanceBaseCache.InstanceBaseLevelMap;

	if (Ar.IsTransacting())
	{
		Ar << InstanceBaseCache.InstanceBaseInvMap;
	}
	else if (Ar.IsLoading())
	{
		InstanceBaseCache.InstanceBaseInvMap.Empty();
		for (const auto& Pair : InstanceBaseCache.InstanceBaseMap)
		{
			const FFoliageInstanceBaseInfo& BaseInfo = Pair.Value;
			InstanceBaseCache.InstanceBaseInvMap.Add(BaseInfo.BasePtr, Pair.Key);
		}
	}
	
	check(InstanceBaseCache.InstanceBaseInvMap.Num() == InstanceBaseCache.InstanceBaseMap.Num());

	return Ar;
}

FFoliageInstanceBaseId FFoliageInstanceBaseCache::AddInstanceBaseId(UActorComponent* InComponent)
{
	FFoliageInstanceBaseId BaseId = FFoliageInstanceBaseCache::InvalidBaseId;
	if (InComponent && InComponent->CreationMethod != EComponentCreationMethod::ConstructionScript)
	{
		BaseId = GetInstanceBaseId(InComponent);
		if (BaseId == FFoliageInstanceBaseCache::InvalidBaseId)
		{
			BaseId = NextBaseId++;
			FFoliageInstanceBasePtr BasePtr = InComponent;

			InstanceBaseMap.Add(BaseId, FFoliageInstanceBaseInfo(InComponent));
			InstanceBaseInvMap.Add(BasePtr, BaseId);

			check(InstanceBaseMap.Num() == InstanceBaseInvMap.Num());

			ULevel* ComponentLevel = InComponent->GetComponentLevel();
			if (ComponentLevel)
			{
				UWorld* ComponentWorld = Cast<UWorld>(ComponentLevel->GetOuter());
				if (ComponentWorld)
				{
					auto WorldKey = TAssetPtr<UWorld>(ComponentWorld);
					InstanceBaseLevelMap.FindOrAdd(WorldKey).Add(BasePtr);
				}
			}
		}
	}

	return BaseId;
}

FFoliageInstanceBaseId FFoliageInstanceBaseCache::GetInstanceBaseId(UActorComponent* InComponent) const
{
	FFoliageInstanceBasePtr BasePtr = InComponent;
	return GetInstanceBaseId(BasePtr);
}

FFoliageInstanceBaseId FFoliageInstanceBaseCache::GetInstanceBaseId(const FFoliageInstanceBasePtr& BasePtr) const
{
	const FFoliageInstanceBaseId* BaseId = InstanceBaseInvMap.Find(BasePtr);
	if (!BaseId)
	{
		return FFoliageInstanceBaseCache::InvalidBaseId;
	}
	
	return *BaseId;
}

FFoliageInstanceBasePtr FFoliageInstanceBaseCache::GetInstanceBasePtr(FFoliageInstanceBaseId BaseId) const
{
	return GetInstanceBaseInfo(BaseId).BasePtr;
}

FFoliageInstanceBaseInfo FFoliageInstanceBaseCache::GetInstanceBaseInfo(FFoliageInstanceBaseId BaseId) const
{
	return InstanceBaseMap.FindRef(BaseId);
}

FFoliageInstanceBaseInfo FFoliageInstanceBaseCache::UpdateInstanceBaseInfoTransform(UActorComponent* InComponent)
{
	auto BaseId = GetInstanceBaseId(InComponent);
	if (BaseId != FFoliageInstanceBaseCache::InvalidBaseId)
	{
		auto* BaseInfo = InstanceBaseMap.Find(BaseId);
		check(BaseInfo);
		BaseInfo->UpdateLocationFromComponent(InComponent);
		return *BaseInfo;
	}

	return FFoliageInstanceBaseInfo();
}

void FFoliageInstanceBaseCache::CompactInstanceBaseCache(AInstancedFoliageActor* IFA)
{
	UWorld* World = IFA->GetWorld();
	if (!World)
	{
		return;
	}

	TSet<FFoliageInstanceBaseId> BasesInUse;
	for (auto& Pair : IFA->FoliageMeshes)
	{
		for (const auto& Pair : Pair.Value->ComponentHash)
		{
			if (Pair.Key != FFoliageInstanceBaseCache::InvalidBaseId)
			{
				BasesInUse.Add(Pair.Key);
			}
		}
	}
	
	// Look for any removed maps
	TSet<FFoliageInstanceBasePtr> InvalidBasePtrs;
	for (auto& Pair : InstanceBaseLevelMap)
	{
		const auto& WorldAsset = Pair.Key;
		
		bool bExists = (WorldAsset == World);
		// Check sub-levels
		if (!bExists)
		{
			const FName PackageName = FName(*FPackageName::ObjectPathToPackageName(WorldAsset.ToStringReference().ToString()));
			if (World->WorldComposition)
			{
				bExists = World->WorldComposition->DoesTileExists(PackageName);
			}
			else
			{
				bExists = (World->GetLevelStreamingForPackageName(PackageName) != nullptr);
			}
		}

		if (!bExists)
		{
			InvalidBasePtrs.Append(Pair.Value);
			InstanceBaseLevelMap.Remove(Pair.Key);
		}
		else
		{
			// Remove dead links
			for (int32 i = Pair.Value.Num()-1; i >= 0; --i)
			{
				// Base needs to be removed if it's not in use by existing instances or component was removed
				if (Pair.Value[i].IsNull() || !BasesInUse.Contains(GetInstanceBaseId(Pair.Value[i])))
				{
					InvalidBasePtrs.Add(Pair.Value[i]);
					Pair.Value.RemoveAt(i);
				}
			}

			if (Pair.Value.Num() == 0)
			{
				InstanceBaseLevelMap.Remove(Pair.Key);
			}
		}
	}
	
	TSet<FFoliageInstanceBaseId> InvalidBaseIds;
	InstanceBaseInvMap.Empty();
	// Look for any removed base components
	for (const auto& Pair : InstanceBaseMap)
	{
		const FFoliageInstanceBaseInfo& BaseInfo = Pair.Value;
		if (InvalidBasePtrs.Contains(BaseInfo.BasePtr))
		{
			InvalidBaseIds.Add(Pair.Key);
			InstanceBaseMap.Remove(Pair.Key);
		}
		else
		{
			// Regenerate inverse map
			check(!InstanceBaseInvMap.Contains(BaseInfo.BasePtr));
			InstanceBaseInvMap.Add(BaseInfo.BasePtr, Pair.Key);
		}
	}

	if (InvalidBaseIds.Num())
	{
		for (auto& Pair : IFA->FoliageMeshes)
		{
			auto& MeshInfo = Pair.Value;
			for (FFoliageInstance& Instance : MeshInfo->Instances)
			{
				if (InvalidBaseIds.Contains(Instance.BaseId))
				{
					Instance.BaseId = FFoliageInstanceBaseCache::InvalidBaseId;
				}
			}

			for (auto RemovedBaseId : InvalidBaseIds)
			{
				MeshInfo->ComponentHash.Remove(RemovedBaseId);
			}
		
			MeshInfo->ComponentHash.Compact();
		}

		InstanceBaseMap.Compact();
		InstanceBaseLevelMap.Compact();
	}
}

void FFoliageInstanceBaseCache::UpdateInstanceBaseCachedTransforms()
{
	for (auto& Pair : InstanceBaseMap)
	{
		FFoliageInstanceBaseInfo& BaseInfo = Pair.Value;
		BaseInfo.UpdateLocationFromComponent(BaseInfo.BasePtr.Get());
	}
}

#endif//WITH_EDITORONLY_DATA
