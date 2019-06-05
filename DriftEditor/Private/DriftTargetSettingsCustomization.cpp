// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#include "DriftEditorPrivatePCH.h"

#include "DriftTargetSettingsCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "PropertyEditing.h"

#include "ObjectEditorUtils.h"
#include "IDocumentation.h"
#include "OutputDevice.h"


#define LOCTEXT_NAMESPACE "DriftTargetSettingsCustomization"


static void OnBrowserLinkClicked(const FSlateHyperlinkRun::FMetadata& Metadata)
{
    const FString* url = Metadata.Find(TEXT("href"));
    
    if (url)
    {
        FPlatformProcess::LaunchURL(**url, nullptr, nullptr);
    }
}


TSharedRef<IDetailCustomization> FDriftTargetSettingsCustomization::MakeInstance()
{
	return MakeShareable(new FDriftTargetSettingsCustomization);
}


void FDriftTargetSettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	SavedLayoutBuilder = &DetailLayout;

	IDetailCategoryBuilder& SetupCategory = DetailLayout.EditCategory(TEXT("Account"), FText::GetEmpty(), ECategoryPriority::Variable);

	SetupCategory.AddCustomRow(LOCTEXT("DocumentationInfo", "Documentation Info"), false)
		.WholeRowWidget
		[
			SNew(SBorder)
			.Padding(1)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(FMargin(10, 10, 10, 10))
				.FillWidth(1.0f)
				[
					SNew(SRichTextBlock)
					.Text(LOCTEXT("DocumentationInfoMessage", "<a id=\"browser\" href=\"http://support.kaleo.io\" style=\"HoverOnlyHyperlink\">View the Drift Unreal Plugin documentation here.</> Please login to your Kaleo account to retrieve api keys. If you don't have an account yet, <a id=\"browser\" href=\"https://www.kaleo.io/signup\" style=\"HoverOnlyHyperlink\">please sign up to create your account.</>"))
					.TextStyle(FEditorStyle::Get(), "MessageLog")
					.DecoratorStyleSet(&FEditorStyle::Get())
					.AutoWrapText(true)
					+ SRichTextBlock::HyperlinkDecorator(TEXT("browser"), FSlateHyperlinkRun::FOnClick::CreateStatic(&OnBrowserLinkClicked))
				]
			]
		];
}


#undef LOCTEXT_NAMESPACE
