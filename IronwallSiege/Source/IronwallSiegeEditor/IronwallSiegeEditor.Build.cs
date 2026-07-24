using UnrealBuildTool;

public class IronwallSiegeEditor : ModuleRules
{
    public IronwallSiegeEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "IronwallSiege"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "AssetRegistry",
            "UnrealEd"
        });
    }
}
