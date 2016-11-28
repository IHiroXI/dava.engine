#pragma once

#include "TArc/DataProcessing/DataNode.h"
#include "TArc/DataProcessing/PropertiesHolder.h"

#include "FileSystem/FilePath.h"

#include <QString>
#include <QVector>

class ProjectStructure;
class SpritesPackerModule;
class EditorConfig;

struct MaterialTemplateInfo
{
    DAVA::String name;
    DAVA::String path;
    DAVA::Vector<DAVA::String> qualities;
};

class ProjectManagerData : public DAVA::TArc::DataNode
{
public:
    ProjectManagerData();
    ProjectManagerData(const ProjectManagerData& other) = delete;
    ~ProjectManagerData();

    bool IsOpened() const;

    const DAVA::FilePath& GetProjectPath() const;
    DAVA::FilePath GetDataPath() const;
    DAVA::FilePath GetDataSourcePath() const;
    DAVA::FilePath GetParticlesConfigPath() const;
    DAVA::FilePath GetParticlesDataPath() const;
    DAVA::FilePath GetWorkspacePath() const;

    static DAVA::FilePath CreateProjectPathFromPath(const DAVA::FilePath& pathname);
    const ProjectStructure* GetDataSourceSceneFiles() const;
    const EditorConfig* GetEditorConfig() const;
    const DAVA::Vector<MaterialTemplateInfo>* GetMaterialTemplatesInfo() const;
    DAVA_DEPRECATED(const SpritesPackerModule* GetSpritesModules() const);

    void SetCloseProjectPredicateFunction(const DAVA::Function<bool()>& fn);

public:
    static const DAVA::String ProjectPathProperty;

private:
    friend class ProjectManagerModule;
    friend class ProjectResources;

    std::unique_ptr<ProjectStructure> dataSourceSceneFiles;
    std::unique_ptr<SpritesPackerModule> spritesPacker;
    std::unique_ptr<EditorConfig> editorConfig;
    DAVA::Vector<MaterialTemplateInfo> materialTemplatesInfo;

    DAVA::FilePath projectPath;

    DAVA::Function<bool()> closeProjectPredicate;

    DAVA_VIRTUAL_REFLECTION(ProjectManagerData, DAVA::TArc::DataNode)
    {
        DAVA::ReflectionRegistrator<ProjectManagerData>::Begin()
        .Field(ProjectPathProperty.c_str(), &ProjectManagerData::GetProjectPath, nullptr)
        .End();
    }
};
