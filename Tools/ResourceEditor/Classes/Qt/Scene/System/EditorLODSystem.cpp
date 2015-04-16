﻿// framework
#include "DAVAEngine.h"
#include "EditorLODSystem.h"
#include "Scene/EntityGroup.h"
#include "Entity/SceneSystem.h"
#include "Scene/SceneSignals.h"
#include "Commands2/ChangeLODDistanceCommand.h"
#include "Commands2/CreatePlaneLODCommand.h"
#include "Commands2/DeleteLODCommand.h"
#include "Commands2/CopyLastLODCommand.h"

EditorLODSystem::EditorLODSystem(DAVA::Scene *scene)
    : DAVA::SceneSystem(scene)
    , currentLodsLayersCount(0)
    , forceDistanceEnabled(false)
    , forceDistance(DAVA::LodComponent::MIN_LOD_DISTANCE)
    , forceLayer(DAVA::LodComponent::INVALID_LOD_LAYER)
    , allSceneModeEnabled(false)
    , mixedForce(false)
{
}

EditorLODSystem::~EditorLODSystem(void)
{
}

void EditorLODSystem::AddEntity(DAVA::Entity * entity)
{
    DVASSERT(entity);
    DAVA::LodComponent *tmpComponent = GetLodComponent(entity);
    DVASSERT(tmpComponent);

    sceneLODsQMap.insert(tmpComponent, ForceData());
}

void EditorLODSystem::RemoveEntity(DAVA::Entity * entity)
{
    DVASSERT(entity);
    DAVA::LodComponent *tmpComponent = GetLodComponent(entity);
    DVASSERT(tmpComponent);

    sceneLODsQMap.remove(tmpComponent);
}

void EditorLODSystem::AddSelectedLODsRecursive(DAVA::Entity *entity)
{
    DVASSERT(entity);
    DAVA::LodComponent *tmpComponent = GetLodComponent(entity);
    if (tmpComponent)
    {
        selectedLODs.append(tmpComponent);
    }
    if (!entity->GetSolid())
    {
        return;
    }
    DAVA::int32 count = entity->GetChildrenCount();
    for (DAVA::int32 i = 0; i < count; ++i)
    {
        AddSelectedLODsRecursive(entity->GetChild(i));
    }
}

void EditorLODSystem::RemoveSelectedLODsRecursive(DAVA::Entity *entity)
{
    DVASSERT(entity);
    DAVA::LodComponent *tmpComponent = GetLodComponent(entity);
    if (tmpComponent)
    {
        selectedLODs.removeOne(tmpComponent);
    }
    DAVA::int32 count = entity->GetChildrenCount();
    for (DAVA::int32 i = 0; i < count; ++i)
    {
        RemoveSelectedLODsRecursive(entity->GetChild(i));
    }
}

void EditorLODSystem::UpdateDistances(const DAVA::Map<DAVA::uint32, DAVA::float32> & newDistances)
{
    if (GetCurrentLODs().isEmpty() || newDistances.empty())
    {
        return;
    }
    for (auto &newDistance : newDistances)
    {
        SetLayerDistance(newDistance.first, newDistance.second);
    }
}

void EditorLODSystem::SceneSelectionChanged(const EntityGroup *selected, const EntityGroup *deselected)
{
    if (!allSceneModeEnabled)
    {
        size_t deselectedSize = deselected->Size();
        for (size_t i = 0; i < deselectedSize; ++i)
        {
            ResetForceState(deselected->GetEntity(i));
        }
    }
    selectedLODs.clear();
    size_t selectedSize = selected->Size();
    if (selectedSize > 1)
    {
        mixedForce = true;
    }
    else
    {
        mixedForce = false;
    }

    if (selectedSize == 0)
    {
        forceDistance = LodComponent::INVALID_DISTANCE;
        forceLayer = LodComponent::INVALID_LOD_LAYER;
    }

    for (size_t i = 0; i < selectedSize; ++i)
    {
        AddSelectedLODsRecursive(selected->GetEntity(i));
    }

    if (allSceneModeEnabled)
    {
        return;
    }

    CollectLODDataFromScene();
    UpdateForceData();
}

void EditorLODSystem::ResetForceState(DAVA::Entity *entity)
{
    DVASSERT(entity);
    DAVA::LodComponent *tmpComponent = GetLodComponent(entity);
    if (tmpComponent)
    {
        ResetForceState(tmpComponent);
    }
    DAVA::int32 count = entity->GetChildrenCount();
    for (DAVA::int32 i = 0; i < count; ++i)
    {
        ResetForceState(entity->GetChild(i));
    }
}

void EditorLODSystem::ResetForceState(DAVA::LodComponent *lodComponent)
{
    DVASSERT(lodComponent);
    lodComponent->SetForceDistance(DAVA::LodComponent::INVALID_DISTANCE);
    lodComponent->SetForceLodLayer(DAVA::LodComponent::INVALID_LOD_LAYER);
    lodComponent->currentLod = DAVA::LodComponent::INVALID_LOD_LAYER;
}

void EditorLODSystem::CollectLODDataFromScene()
{
    currentLodsLayersCount = 0;
    std::fill(lodDistances.begin(), lodDistances.end(), 0);
    std::fill(lodTrianglesCount.begin(), lodTrianglesCount.end(), 0);
    std::array<DAVA::int32, DAVA::LodComponent::MAX_LOD_LAYERS> lodsComponentsCount = { 0 };
    const QList<DAVA::LodComponent *> &lods = GetCurrentLODs();
    for (auto &lod : lods)
    {
        DAVA::int32 layersCount = GetLodLayersCount(lod);
        DVASSERT(layersCount <= DAVA::LodComponent::MAX_LOD_LAYERS);
        for (auto layer = 0; layer < layersCount; ++layer)
        {
            lodDistances[layer] += lod->GetLodLayerDistance(layer);
            lodsComponentsCount[layer]++;
        }
        //triangles
        AddTrianglesInfo(lodTrianglesCount, lod, false);
    }
    //distances
    for (auto i = 0; i < DAVA::LodComponent::MAX_LOD_LAYERS; ++i)
    {
        if(0 != lodsComponentsCount[i])
        {
            lodDistances[i] /= lodsComponentsCount[i];
            ++currentLodsLayersCount;
        }
    }
    if (!forceDistanceEnabled && forceLayer >= currentLodsLayersCount)
    {
        SetForceLayer(currentLodsLayersCount - 1);
        return;
    }
}

void EditorLODSystem::AddTrianglesInfo(std::array<DAVA::uint32, DAVA::LodComponent::MAX_LOD_LAYERS> &triangles, DAVA::LodComponent *lod, bool onlyVisibleBatches)
{
    Entity * en = lod->GetEntity();
    if (nullptr != GetEffectComponent(en))
    {
        return;
    }
    RenderObject * ro = GetRenderObject(en);
    if (nullptr == ro)
    {
        return;
    }
    DAVA::uint32 batchCount = ro->GetRenderBatchCount();
    for (DAVA::uint32 i = 0; i < batchCount; ++i)
    {
        DAVA::int32 lodIndex = 0;
        DAVA::int32 switchIndex = 0;

        RenderBatch *rb = ro->GetRenderBatch(i, lodIndex, switchIndex);
        if (lodIndex < 0 || lodIndex >= DAVA::LodComponent::MAX_LOD_LAYERS)
        {
            Logger::Error("got unexpected lod index (%d) when collecting triangles on entitie %s. Correct values for lod index is %d", lodIndex, en->GetName().c_str(), DAVA::LodComponent::MAX_LOD_LAYERS);
            continue;
        }
    
        if(IsPointerToExactClass<RenderBatch>(rb))
        {
            if(onlyVisibleBatches)
            { //check batch visibility

                bool batchIsVisible = false;
                DAVA::uint32 activeBatchCount = ro->GetActiveRenderBatchCount();
                for (DAVA::uint32 a = 0; a < activeBatchCount && !batchIsVisible; ++a)
                {
                    RenderBatch *visibleBatch = ro->GetActiveRenderBatch(a);
                    batchIsVisible = (visibleBatch == rb);
                }

                if (batchIsVisible == false) // need to skip this render batch
                {
                    continue;
                }
            }

            PolygonGroup *pg = rb->GetPolygonGroup();
            if(nullptr != pg)
            {
                DVASSERT(lodIndex < DAVA::LodComponent::MAX_LOD_LAYERS);
                DVASSERT(lodIndex >= 0);
                triangles[lodIndex] += pg->GetIndexCount() / 3; 
            }
        }
    }
}

bool EditorLODSystem::CheckSelectedContainsEntity(const DAVA::Entity *arg) const
{
    DVASSERT(arg);
    if (allSceneModeEnabled)
    {
        return true;
    }

    for (DAVA::LodComponent *lod : selectedLODs)
    {
        const Entity *entity = lod->GetEntity();
        if (entity == arg)
        {
            return true;
        }
    }
    return false;
}

void EditorLODSystem::SolidChanged(const Entity *entity, bool value)
{
    DVASSERT(entity);
    if (!CheckSelectedContainsEntity(entity))
    {
        return;
    }

    if (value)
    {
        DAVA::int32 count = entity->GetChildrenCount();
        for (DAVA::int32 i = 0; i < count; ++i)
        {
            AddSelectedLODsRecursive(entity->GetChild(i));
        }
    }
    else
    {
        DAVA::int32 count = entity->GetChildrenCount();
        for (DAVA::int32 i = 0; i < count; ++i)
        {
            RemoveSelectedLODsRecursive(entity->GetChild(i));
        }
    }
    CollectLODDataFromScene();
}


bool EditorLODSystem::CanCreatePlaneLOD() const
{
    if (1 != GetCurrentLODs().size())
    {
        return false;
    }
    for (auto &lod : GetCurrentLODs())
    {
        if (lod->GetEntity()->GetComponent(Component::PARTICLE_EFFECT_COMPONENT))
        {
            return false;
        }
    }
    return (GetLodLayersCount(selectedLODs.last()->GetEntity()) < LodComponent::MAX_LOD_LAYERS);
}

bool EditorLODSystem::CreatePlaneLOD(DAVA::int32 fromLayer, DAVA::uint32 textureSize, const DAVA::FilePath & texturePath)
{
    if (GetCurrentLODs().isEmpty())
    {
        return false;
    }

    SceneEditor2* sceneEditor2 = static_cast<SceneEditor2*>(GetScene());

    sceneEditor2->BeginBatch("LOD Added");

    for (auto &lod : GetCurrentLODs())
    {
        sceneEditor2->Exec(new CreatePlaneLODCommand(lod, fromLayer, textureSize, texturePath));
    }
    sceneEditor2->EndBatch();
    return true;
}

bool EditorLODSystem::CopyLastLodToLod0()
{
    if (GetCurrentLODs().isEmpty())
    {
        return false;
    }    
    SceneEditor2* sceneEditor2 = static_cast<SceneEditor2*>(GetScene());

    sceneEditor2->BeginBatch("LOD Added");

    for (auto &lod : GetCurrentLODs())
    {
        sceneEditor2->Exec(new CopyLastLODToLod0Command(lod));
    }
    sceneEditor2->EndBatch();
    return true;
}

FilePath EditorLODSystem::GetDefaultTexturePathForPlaneEntity() const
{
    DVASSERT(GetCurrentLODs().size() == 1);
    Entity * entity = GetCurrentLODs().last()->GetEntity();

    FilePath entityPath = static_cast<SceneEditor2*>(GetScene())->GetScenePath();
    KeyedArchive * properties = GetCustomPropertiesArchieve(entity);
    if (nullptr != properties && properties->IsKeyExists(ResourceEditor::EDITOR_REFERENCE_TO_OWNER))
    {
        entityPath = FilePath(properties->GetString(ResourceEditor::EDITOR_REFERENCE_TO_OWNER, entityPath.GetAbsolutePathname()));
    }
    String entityName = entity->GetName().c_str();
    FilePath textureFolder = entityPath.GetDirectory() + "images/";

    String texturePostfix = "_planes.png";
    FilePath texturePath = textureFolder + entityName + texturePostfix;
    int32 i = 0;
    while(texturePath.Exists())
    {
        i++;
        texturePath = textureFolder + Format("%s_%d%s", entityName.c_str(), i, texturePostfix.c_str());
    }

    return texturePath;
}

bool EditorLODSystem::CanDeleteLod() const
{
    if (GetCurrentLODs().isEmpty())
    {
        return false;
    }
    bool containLayers = false;
    for (auto &lod : GetCurrentLODs())
    {
        if (nullptr != lod->GetEntity()->GetComponent(Component::PARTICLE_EFFECT_COMPONENT))
        {
            return false;
        }
        else if (!containLayers && GetLodLayersCount(lod) > 1)
        {
            containLayers = true;
        }
    }
    return containLayers;
}

bool EditorLODSystem::DeleteFirstLOD()
{
    if (false == CanDeleteLod())
    {
        return false;
    }
    SceneEditor2* sceneEditor2 = static_cast<SceneEditor2*>(GetScene());
    sceneEditor2->BeginBatch("Delete First LOD");
    for (auto &lod : GetCurrentLODs())
    {
        if (GetLodLayersCount(lod) > 1)
        {
            sceneEditor2->Exec(new DeleteLODCommand(lod, 0, -1));
        }
    }
    sceneEditor2->EndBatch();
    return true;
}

bool EditorLODSystem::DeleteLastLOD()
{
    if (false == CanDeleteLod())
    {
        return false;
    }
    SceneEditor2* sceneEditor2 = static_cast<SceneEditor2*>(GetScene());
    sceneEditor2->BeginBatch("Delete Last LOD");
    for (auto &lod : GetCurrentLODs())
    {
        if (GetLodLayersCount(lod) > 1)
        {
            sceneEditor2->Exec(new DeleteLODCommand(lod, GetLodLayersCount(lod) - 1, -1));
        }
    }
    sceneEditor2->EndBatch();
    return true;
}

void EditorLODSystem::SetLayerDistance(DAVA::int32 layerNum, DAVA::float32 distance)
{
    DVASSERT(layerNum < currentLodsLayersCount);
    lodDistances[layerNum] = distance;
    if (GetCurrentLODs().isEmpty())
    {
        return;
    }
    SceneEditor2* sceneEditor2 = static_cast<SceneEditor2*>(GetScene());
    sceneEditor2->BeginBatch("LOD Distance Changed");
    for (auto &lod : GetCurrentLODs())
    {
        sceneEditor2->Exec(new ChangeLODDistanceCommand(lod, layerNum, distance));
    }
    sceneEditor2->EndBatch();
    CollectLODDataFromScene();
}

void EditorLODSystem::SetForceDistanceEnabled(bool enable)
{
    if (forceDistanceEnabled == enable)
    {
        return;
    }
    forceDistanceEnabled = enable;
    UpdateForceData();
}

void EditorLODSystem::UpdateForceData()
{
    if (forceDistanceEnabled)
    {
        UpdateForceDistance();
    }
    else
    {
        UpdateForceLayer();
    }
}

void EditorLODSystem::SetForceDistance(DAVA::float32 distance)
{
    forceDistance = distance;

    for (LodComponent *lod : GetCurrentLODs())
    {
        lod->SetForceLodLayer(LodComponent::INVALID_LOD_LAYER);
        lod->currentLod = DAVA::LodComponent::INVALID_LOD_LAYER;
        lod->SetForceDistance(distance);

        if (!allSceneModeEnabled && SettingsManager::GetValue(Settings::Scene_RememberForceParameters).AsBool())
        {
            ForceData force = sceneLODsQMap.value(lod);
            force.forceDistance = distance;
            sceneLODsQMap.insert(lod, force);
        }
    }
}

DAVA::float32 EditorLODSystem::GetCurrentDistance() const
{
    if (!SettingsManager::GetValue(Settings::Scene_RememberForceParameters).AsBool())
    {
        return (forceDistance < 0) ? LodComponent::MIN_LOD_DISTANCE : forceDistance;
    }

    if (GetCurrentLODs().isEmpty())
    {
        return DAVA::LodComponent::MIN_LOD_DISTANCE;
    }

    if (mixedForce)
    {
        return DAVA::LodComponent::MIN_LOD_DISTANCE;
    }
    else
    {
        DAVA::float32 dist = sceneLODsQMap.first().forceDistance;
        return (dist < 0) ? LodComponent::MIN_LOD_DISTANCE : dist;
    }
}

void EditorLODSystem::UpdateForceDistance()
{
    DAVA::Map<int, int> m;

    if (!SettingsManager::GetValue(Settings::Scene_RememberForceParameters).AsBool())
    {
        for (LodComponent *lod : GetCurrentLODs())
        {
            lod->SetForceLodLayer(LodComponent::INVALID_LOD_LAYER);
            lod->currentLod = DAVA::LodComponent::INVALID_LOD_LAYER;
            lod->SetForceDistance(forceDistance);
        }
        return;
    }

    if (allSceneModeEnabled)
    {
        return;
    }

    for (LodComponent *lod : GetCurrentLODs())
    {
        lod->SetForceLodLayer(LodComponent::INVALID_LOD_LAYER);
        lod->currentLod = DAVA::LodComponent::INVALID_LOD_LAYER;
        lod->SetForceDistance(sceneLODsQMap.value(lod).forceDistance);
    }
}

void EditorLODSystem::SetForceLayer(DAVA::int32 layer)
{
    forceLayer = layer;

    for (LodComponent *lod : GetCurrentLODs())
    {
        lod->SetForceDistance(DAVA::LodComponent::INVALID_DISTANCE);
        lod->currentLod = DAVA::LodComponent::INVALID_LOD_LAYER;
        lod->SetForceLodLayer(layer);

        if (!allSceneModeEnabled && SettingsManager::GetValue(Settings::Scene_RememberForceParameters).AsBool())
        {
            ForceData force = sceneLODsQMap.value(lod);
            force.forceLayer = layer;
            sceneLODsQMap.insert(lod, force);
        }
    }
}

DAVA::int32 EditorLODSystem::GetCurrentForceLayer() const
{
    if (!SettingsManager::GetValue(Settings::Scene_RememberForceParameters).AsBool())
    {
        return forceLayer;
    }

    if (GetCurrentLODs().isEmpty())
    {
        return DAVA::LodComponent::INVALID_LOD_LAYER;
    }

    if (mixedForce)
    {
        return DAVA::LodComponent::MAX_LOD_LAYERS;
    }
    else
    {
        return sceneLODsQMap.first().forceLayer;
    }
}

void EditorLODSystem::UpdateForceLayer()
{
    if (!SettingsManager::GetValue(Settings::Scene_RememberForceParameters).AsBool())
    {
        for (LodComponent *lod : GetCurrentLODs())
        {
            lod->SetForceDistance(DAVA::LodComponent::INVALID_DISTANCE);
            lod->currentLod = DAVA::LodComponent::INVALID_LOD_LAYER;
            lod->SetForceLodLayer(forceLayer);
        }
        return;
    }

    if (allSceneModeEnabled)
    {
        return;
    }

    for (LodComponent *lod : GetCurrentLODs())
    {
        lod->SetForceDistance(DAVA::LodComponent::INVALID_DISTANCE);
        lod->currentLod = DAVA::LodComponent::INVALID_LOD_LAYER;
        lod->SetForceLodLayer(sceneLODsQMap.value(lod).forceLayer);
    }
}

void EditorLODSystem::SetAllSceneModeEnabled(bool enabled)
{
    if (allSceneModeEnabled == enabled)
    {
        return;
    }
    allSceneModeEnabled = enabled;
    UpdateAllSceneModeEnabled();
}

void EditorLODSystem::UpdateAllSceneModeEnabled()
{
    for (DAVA::LodComponent *lod : sceneLODsQMap.keys())
    {
        ResetForceState(lod);
    }
    CollectLODDataFromScene();
    UpdateForceData();
}
