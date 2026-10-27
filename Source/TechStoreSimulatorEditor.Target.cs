using UnrealBuildTool;
using System.Collections.Generic;

public class TechStoreSimulatorEditorTarget : TargetRules
{
	public TechStoreSimulatorEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("TechStoreSimulator");
	}
}