// Copyright 2014 Michael Allar. All Rights Reserved.

#include "ExportObjectPluginPrivatePCH.h"
#include "StereoscopicRenderBlueprintLibrary.h"
#include "ObjectTools.h"



bool UStereoscopicRenderBlueprintLibrary::ExportObjectToPath(class UObject* ObjectToExport, FString SaveFileName, FString FileNameAppendage)
{
	if (ObjectToExport == nullptr)
	{
		UE_LOG(LogExportObjectPlugin, Warning, TEXT("Export Object to Path called without a valid object!"));
		return false;
	}


	// Create an array of all available exporters.
	TArray<UExporter*> Exporters;
	ObjectTools::AssembleListOfExporters(Exporters);

	// Find all the exporters that can export this type of object and construct an export file dialog.
	TArray<FString> AllFileTypes;
	TArray<FString> AllExtensions;
	TArray<FString> PreferredExtensions;

	// Iterate in reverse so the most relevant file formats are considered first.
	for (int32 ExporterIndex = Exporters.Num() - 1; ExporterIndex >= 0; --ExporterIndex)
	{
		UExporter* Exporter = Exporters[ExporterIndex];
		if (Exporter->SupportedClass)
		{
			const bool bObjectIsSupported = Exporter->SupportsObject(ObjectToExport);
			if (bObjectIsSupported)
			{
				// Get a string representing of the exportable types.
				check(Exporter->FormatExtension.Num() == Exporter->FormatDescription.Num());
				check(Exporter->FormatExtension.IsValidIndex(Exporter->PreferredFormatIndex));
				for (int32 FormatIndex = Exporter->FormatExtension.Num() - 1; FormatIndex >= 0; --FormatIndex)
				{
					const FString& FormatExtension = Exporter->FormatExtension[FormatIndex];
					const FString& FormatDescription = Exporter->FormatDescription[FormatIndex];

					if (FormatIndex == Exporter->PreferredFormatIndex)
					{
						PreferredExtensions.Add(FormatExtension);
					}
					AllFileTypes.Add(FString::Printf(TEXT("%s (*.%s)|*.%s"), *FormatDescription, *FormatExtension, *FormatExtension));
					AllExtensions.Add(FString::Printf(TEXT("*.%s"), *FormatExtension));
				}
			}
		}
	}

	// Skip this object if no exporter found for this resource type.
	if (PreferredExtensions.Num() == 0)
	{
		UE_LOG(LogExportObjectPlugin, Warning, TEXT("Export Object to Path unable to find exporter for %s!"), *ObjectToExport->GetName());
		return false;
	}

	if (!FPackageName::IsShortPackageName(ObjectToExport->GetOutermost()->GetFName()))
	{
		// Determine the save file name from the long package name
		FString PackageName = ObjectToExport->GetOutermost()->GetName();
		if (PackageName.Left(1) == TEXT("/"))
		{
			// Trim the leading slash so the file manager doesn't get confused
			PackageName = PackageName.Mid(1);
		}

		FPaths::NormalizeFilename(PackageName);
		SaveFileName /= PackageName;
	}
	else
	{
		// Assemble the path from the package name.
		SaveFileName /= ObjectToExport->GetOutermost()->GetName();
		SaveFileName /= ObjectToExport->GetName();
	}

	FString FirstExtension = PreferredExtensions[0];
	SaveFileName += FileNameAppendage + FString::Printf(TEXT(".%s"), *FirstExtension);

	UE_LOG(LogExportObjectPlugin, Log, TEXT("ExportObjectToPath Exporting \"%s\" to \"%s\""), *ObjectToExport->GetPathName(), *SaveFileName);

	// Create the path, then make sure the target file is not read-only.
	const FString ObjectExportPath(FPaths::GetPath(SaveFileName));
	const bool bFileInSubdirectory = ObjectExportPath.Contains(TEXT("/"));
	if (bFileInSubdirectory && (!IFileManager::Get().MakeDirectory(*ObjectExportPath, true)))
	{
		UE_LOG(LogExportObjectPlugin, Warning, TEXT("ExportObjectToPath Failed to make directory %s"), *ObjectExportPath);
		return false;
	}
	else if (IFileManager::Get().IsReadOnly(*SaveFileName))
	{
		UE_LOG(LogExportObjectPlugin, Warning, TEXT("ExportObjectToPath Couldn't write to file '%s'. Maybe file is read-only?"), *ObjectExportPath);
		return false;
	}
	else
	{
		// We have a writeable file.  Now go through that list of exporters again and find the right exporter and use it.
		TArray<UExporter*>	ValidExporters;

		for (int32 ExporterIndex = 0; ExporterIndex < Exporters.Num(); ++ExporterIndex)
		{
			UExporter* Exporter = Exporters[ExporterIndex];
			if (Exporter->SupportsObject(ObjectToExport))
			{
				check(Exporter->FormatExtension.Num() == Exporter->FormatDescription.Num());
				for (int32 FormatIndex = 0; FormatIndex < Exporter->FormatExtension.Num(); ++FormatIndex)
				{
					const FString& FormatExtension = Exporter->FormatExtension[FormatIndex];
					if (FCString::Stricmp(*FormatExtension, *FPaths::GetExtension(SaveFileName)) == 0 ||
						FCString::Stricmp(*FormatExtension, TEXT("*")) == 0)
					{
						ValidExporters.Add(Exporter);
						break;
					}
				}
			}
		}

		// Handle the potential of multiple exporters being found
		UExporter* ExporterToUse = NULL;
		if (ValidExporters.Num() == 1)
		{
			ExporterToUse = ValidExporters[0];
		}
		else if (ValidExporters.Num() > 1)
		{
			// Set up the first one as default
			ExporterToUse = ValidExporters[0];

			// ...but search for a better match if available
			for (int32 ExporterIdx = 0; ExporterIdx < ValidExporters.Num(); ExporterIdx++)
			{
				if (ValidExporters[ExporterIdx]->GetClass()->GetFName() == ObjectToExport->GetExporterName())
				{
					ExporterToUse = ValidExporters[ExporterIdx];
					break;
				}
			}
		}

		// If an exporter was found, use it.
		if (ExporterToUse)
		{
			UExporter::FExportToFileParams Params;
			Params.Object = ObjectToExport;
			Params.Exporter = ExporterToUse;
			Params.Filename = *SaveFileName;
			Params.InSelectedOnly = false;
			Params.NoReplaceIdentical = false;
			Params.Prompt = false;
			Params.bUseFileArchive = ObjectToExport->IsA(UPackage::StaticClass());
			Params.WriteEmptyFiles = false;
			UExporter::ExportToFileEx(Params);

			return true;
		}
		else
		{
			UE_LOG(LogExportObjectPlugin, Warning, TEXT("ExportObjectToPath Failed to find exporter (last stage) for object: %s"), *ObjectToExport->GetName());
		}
	}


	UE_LOG(LogExportObjectPlugin, Warning, TEXT("ExportObjectToPath Failed for unknown reason for object: %s"), *ObjectToExport->GetName());
	return false;

	
}

