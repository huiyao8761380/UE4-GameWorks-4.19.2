// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "PropertyEditing.h"
#include "ObjectEditorUtils.h"
#include "WidgetGraphSchema.h"
#include "ScopedTransaction.h"
#include "BlueprintEditorUtils.h"

#include "SlateChildSizeCustomization.h"

#define LOCTEXT_NAMESPACE "UMG"

void FSlateChildSizeCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TSharedPtr<IPropertyHandle> ValueHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSlateChildSize, Value));
	TSharedPtr<IPropertyHandle> RuleHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSlateChildSize, SizeRule));

	const FMargin OuterPadding(2);
	const FMargin ContentPadding(2);

	if ( !( ValueHandle.IsValid() || RuleHandle.IsValid() ) )
	{
		return;
	}

	HeaderRow
	.NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(OuterPadding)
		[
			SNew(SCheckBox)
			.Style(FEditorStyle::Get(), "ToggleButtonCheckbox")
			.ToolTipText(LOCTEXT("Auto_ToolTip", "Auto"))
			.Padding(ContentPadding)
			.OnCheckStateChanged(this, &FSlateChildSizeCustomization::HandleCheckStateChanged, RuleHandle, ESlateSizeRule::Automatic)
			.IsChecked(this, &FSlateChildSizeCustomization::GetCheckState, RuleHandle, ESlateSizeRule::Automatic)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Auto", "Auto"))
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(OuterPadding)
		[
			SNew(SCheckBox)
			.Style(FEditorStyle::Get(), "ToggleButtonCheckbox")
			.ToolTipText(LOCTEXT("Fill_ToolTip", "Fill"))
			.Padding(ContentPadding)
			.OnCheckStateChanged(this, &FSlateChildSizeCustomization::HandleCheckStateChanged, RuleHandle, ESlateSizeRule::Fill)
			.IsChecked(this, &FSlateChildSizeCustomization::GetCheckState, RuleHandle, ESlateSizeRule::Fill)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Fill", "Fill"))
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(OuterPadding)
		[
			SNew(SBox)
			.WidthOverride(40)
			[
				SNew( SNumericEntryBox<float> )
				.LabelVAlign(VAlign_Center)
				.Visibility(this, &FSlateChildSizeCustomization::GetValueVisiblity, RuleHandle)
				//.Label()
				//[
				//	SNew(STextBlock)
				//	.Text(LOCTEXT("Fill", "Fill"))
				//]
				.Value(this, &FSlateChildSizeCustomization::GetValue, ValueHandle)
				.OnValueCommitted(this, &FSlateChildSizeCustomization::HandleValueComitted, ValueHandle)
				.UndeterminedString( LOCTEXT("MultipleValues", "Multiple Values") )
			]
		]
	];
}

void FSlateChildSizeCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	
}

void FSlateChildSizeCustomization::HandleCheckStateChanged(ESlateCheckBoxState::Type InCheckboxState, TSharedPtr<IPropertyHandle> PropertyHandle, ESlateSizeRule::Type ToRule)
{
	PropertyHandle->SetValue((uint8)ToRule);
}

ESlateCheckBoxState::Type FSlateChildSizeCustomization::GetCheckState(TSharedPtr<IPropertyHandle> PropertyHandle, ESlateSizeRule::Type ForRule) const
{
	uint8 Value;
	if ( PropertyHandle->GetValue(Value) )
	{
		return Value == ForRule ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}

	return ESlateCheckBoxState::Unchecked;
}

TOptional<float> FSlateChildSizeCustomization::GetValue(TSharedPtr<IPropertyHandle> ValueHandle) const
{
	float Value;
	if ( ValueHandle->GetValue(Value) )
	{
		return Value;
	}

	return TOptional<float>();
}

void FSlateChildSizeCustomization::HandleValueComitted(float NewValue, ETextCommit::Type CommitType, TSharedPtr<IPropertyHandle> ValueHandle)
{
	ValueHandle->SetValue(NewValue);
}

EVisibility FSlateChildSizeCustomization::GetValueVisiblity(TSharedPtr<IPropertyHandle> RuleHandle) const
{
	uint8 Value;
	if ( RuleHandle->GetValue(Value) )
	{
		return Value == ESlateSizeRule::Fill ? EVisibility::Visible : EVisibility::Collapsed;
	}

	return EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE
