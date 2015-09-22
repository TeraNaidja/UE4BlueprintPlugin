// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"

#include "CompilerResultsLog.h"
#include "ClassIconFinder.h"
#include "MessageLog.h"

#define LOCTEXT_NAMESPACE "K2Node"

UK2Node_Variable::UK2Node_Variable(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UK2Node_Variable::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	// Fix old content 
	if(Ar.IsLoading())
	{
		if(Ar.UE4Ver() < VER_UE4_VARK2NODE_USE_MEMBERREFSTRUCT)
		{
			// Copy info into new struct
			VariableReference.SetDirect(VariableName_DEPRECATED, FGuid(), VariableSourceClass_DEPRECATED, bSelfContext_DEPRECATED);
		}

		if(Ar.UE4Ver() < VER_UE4_K2NODE_REFERENCEGUIDS)
		{
			FGuid VarGuid;
			
			if (UBlueprint::GetGuidFromClassByFieldName<UProperty>(GetBlueprint()->GeneratedClass, VariableReference.GetMemberName(), VarGuid))
			{
				const bool bSelf = VariableReference.IsSelfContext();
				VariableReference.SetDirect(VariableReference.GetMemberName(), VarGuid, (bSelf ? NULL : VariableReference.GetMemberParentClass((UClass*)NULL)), bSelf);
			}
		}
	}
}

void UK2Node_Variable::SetFromProperty(const UProperty* Property, bool bSelfContext)
{
	SelfContextInfo = bSelfContext ? ESelfContextInfo::Unspecified : ESelfContextInfo::NotSelfContext;
	VariableReference.SetFromField<UProperty>(Property, bSelfContext);
}

bool UK2Node_Variable::CreatePinForVariable(EEdGraphPinDirection Direction, FString InPinName/* = FString()*/)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UProperty* VariableProperty = GetPropertyForVariable();
	// favor the skeleton property if possible (in case the property type has 
	// been changed, and not yet compiled).
	if (!VariableReference.IsSelfContext())
	{
		UClass* VariableClass = VariableReference.GetMemberParentClass(GetBlueprint()->GeneratedClass);
		if (UBlueprintGeneratedClass* BpClassOwner = Cast<UBlueprintGeneratedClass>(VariableClass))
		{
			// this variable could currently only be a part of some skeleton 
			// class (the blueprint has not be compiled with it yet), so let's 
			// check the skeleton class as well, see if we can pull pin data 
			// from there...
			UBlueprint* VariableBlueprint = CastChecked<UBlueprint>(BpClassOwner->ClassGeneratedBy, ECastCheckedType::NullAllowed);
			if (VariableBlueprint)
			{
				if (UProperty* SkelProperty = FindField<UProperty>(VariableBlueprint->SkeletonGeneratedClass, VariableReference.GetMemberName()))
				{
					VariableProperty = SkelProperty;
				}
			}
		}
	}

	if (VariableProperty != NULL)
	{
		const FString PinName = InPinName.IsEmpty()? GetVarNameString() : InPinName;
		// Create the pin
		UEdGraphPin* VariablePin = CreatePin(Direction, TEXT(""), TEXT(""), NULL, false, false, PinName);
		K2Schema->ConvertPropertyToPinType(VariableProperty, /*out*/ VariablePin->PinType);
		K2Schema->SetPinDefaultValueBasedOnType(VariablePin);
	}
	else
	{
		if (!VariableReference.IsLocalScope())
		{
			Message_Warn(*FString::Printf(TEXT("CreatePinForVariable: '%s' variable not found. Base class was probably changed."), *GetVarNameString()));
		}
		return false;
	}

	return true;
}

void UK2Node_Variable::CreatePinForSelf()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	// Create the self pin
	if (!K2Schema->FindSelfPin(*this, EGPD_Input))
	{
		// Do not create a self pin for locally scoped variables
		if( !VariableReference.IsLocalScope() )
		{
			bool bSelfTarget = VariableReference.IsSelfContext() && (ESelfContextInfo::NotSelfContext != SelfContextInfo);
			UClass* MemberParentClass = VariableReference.GetMemberParentClass(this);
			UClass* TargetClass = MemberParentClass;
			
			// Self Target pins should always make the class be the owning class of the property,
			// so if the node is from a Macro Blueprint, it will hook up as self in any placed Blueprint
			if(bSelfTarget)
			{
				if(UProperty* Property = VariableReference.ResolveMember<UProperty>(this))
				{
					TargetClass = Property->GetOwnerClass()->GetAuthoritativeClass();
				}
				else
				{
					TargetClass = GetBlueprint()->SkeletonGeneratedClass->GetAuthoritativeClass();
				}
			}
			else if(MemberParentClass && MemberParentClass->ClassGeneratedBy)
			{
				TargetClass = MemberParentClass->GetAuthoritativeClass();
			}

			UEdGraphPin* TargetPin = CreatePin(EGPD_Input, K2Schema->PC_Object, TEXT(""), TargetClass, false, false, K2Schema->PN_Self);
			TargetPin->PinFriendlyName =  LOCTEXT("Target", "Target");

			if (bSelfTarget)
			{
				TargetPin->bHidden = true; // don't show in 'self' context
			}
		}
	}
	else
	{
		//@TODO: Check that the self pin types match!
	}
}

bool UK2Node_Variable::RecreatePinForVariable(EEdGraphPinDirection Direction, TArray<UEdGraphPin*>& OldPins, FString InPinName/* = FString()*/)
{
	// probably the node was pasted to a blueprint without the variable
	// we don't want to beak any connection, so the pin will be recreated from old one, but compiler will throw error

	// find old variable pin
	const UEdGraphPin* OldVariablePin = NULL;
	const FString PinName = InPinName.IsEmpty()? GetVarNameString() : InPinName;
	for(auto Iter = OldPins.CreateConstIterator(); Iter; ++Iter)
	{
		if(const UEdGraphPin* Pin = *Iter)
		{
			if(PinName == Pin->PinName)
			{
				OldVariablePin = Pin;
				break;
			}
		}
	}

	if(NULL != OldVariablePin)
	{
		// create new pin from old one
		UEdGraphPin* VariablePin = CreatePin(Direction, TEXT(""), TEXT(""), NULL, false, false, PinName);
		VariablePin->PinType = OldVariablePin->PinType;
		
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
		K2Schema->SetPinDefaultValueBasedOnType(VariablePin);

		Message_Note(*FString::Printf(TEXT("Pin for variable '%s' recreated, but the variable is missing."), *PinName));
		return true;
	}
	else
	{
		Message_Warn(*FString::Printf(TEXT("RecreatePinForVariable: '%s' pin not found"), *PinName));
		return false;
	}
}

FLinearColor UK2Node_Variable::GetNodeTitleColor() const
{
	UProperty* VariableProperty = GetPropertyForVariable();
	if (VariableProperty)
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

		FEdGraphPinType VariablePinType;
		K2Schema->ConvertPropertyToPinType(VariableProperty, VariablePinType);

		return K2Schema->GetPinTypeColor(VariablePinType);
	}

	return FLinearColor::White;
}

UK2Node::ERedirectType UK2Node_Variable::DoPinsMatchForReconstruction( const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex ) const 
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	if( OldPin->PinType.PinCategory == K2Schema->PC_Exec )
	{
		return Super::DoPinsMatchForReconstruction(NewPin, NewPinIndex, OldPin, OldPinIndex);
	}

	const bool bCanMatchSelfs = ((OldPin->PinName == K2Schema->PN_Self) == (NewPin->PinName == K2Schema->PN_Self));
	const bool bTheSameDirection = (NewPin->Direction == OldPin->Direction);
	if (bCanMatchSelfs && bTheSameDirection)
	{
		if (K2Schema->ArePinTypesCompatible(NewPin->PinType, OldPin->PinType))
		{
			// If these are split pins, we need to do some name checking logic
			if (NewPin->ParentPin)
			{
				// If the OldPin is not split, then these don't match
				if (OldPin->ParentPin == nullptr)
				{
					return ERedirectType_None;
				}

				// Go through and find the original variable pin.
				// If the number of steps out to the original variable pin is not the same then these don't match
				const UEdGraphPin* ParentmostNewPin = NewPin;
				const UEdGraphPin* ParentmostOldPin = OldPin;

				while (ParentmostNewPin->ParentPin)
				{
					if (ParentmostOldPin->ParentPin == nullptr)
					{
						return ERedirectType_None;
					}
					ParentmostNewPin = ParentmostNewPin->ParentPin;
					ParentmostOldPin = ParentmostOldPin->ParentPin;
				}

				if (ParentmostOldPin->ParentPin)
				{
					return ERedirectType_None;
				}

				// Compare whether the names, ignoring the original variable's name in the case of renames, match
				FString NewPinPropertyName = NewPin->PinName.RightChop(ParentmostNewPin->PinName.Len() + 1);
				FString OldPinPropertyName = OldPin->PinName.RightChop(ParentmostOldPin->PinName.Len() + 1);

				if (NewPinPropertyName != OldPinPropertyName)
				{
					return ERedirectType_None;
				}
			}

			return ERedirectType_Name;
		}
		else if ((OldPin->PinName == NewPin->PinName) && ((NewPin->PinType.PinCategory == K2Schema->PC_Object) ||
			(NewPin->PinType.PinCategory == K2Schema->PC_Interface)) && (NewPin->PinType.PinSubCategoryObject == NULL))
		{
			// Special Case:  If we had a pin match, and the class isn't loaded yet because of a cyclic dependency, temporarily cast away the const, and fix up.
			// @TODO:  Fix this up to be less hacky
			UBlueprintGeneratedClass* TypeClass = Cast<UBlueprintGeneratedClass>(OldPin->PinType.PinSubCategoryObject.Get());
			if (TypeClass && TypeClass->ClassGeneratedBy && TypeClass->ClassGeneratedBy->HasAnyFlags(RF_BeingRegenerated))
			{
				UEdGraphPin* NonConstNewPin = (UEdGraphPin*)NewPin;
				NonConstNewPin->PinType.PinSubCategoryObject = OldPin->PinType.PinSubCategoryObject.Get();
				return ERedirectType_Name;
			}
		}
		else
		{
			// Special Case:  If we're migrating from old blueprint references to class references, allow pins to be reconnected if coerced
			const UClass* PSCOClass = Cast<UClass>(OldPin->PinType.PinSubCategoryObject.Get());
			const bool bOldIsBlueprint = PSCOClass && PSCOClass->IsChildOf(UBlueprint::StaticClass());
			const bool bNewIsClass = (NewPin->PinType.PinCategory == K2Schema->PC_Class);
			if (bNewIsClass && bOldIsBlueprint)
			{
				UEdGraphPin* OldPinNonConst = (UEdGraphPin*)OldPin;
				OldPinNonConst->PinName = NewPin->PinName;
				return ERedirectType_Name;
			}
		}
	}

	return ERedirectType_None;
}

UClass* UK2Node_Variable::GetVariableSourceClass() const
{
	UClass* Result = VariableReference.GetMemberParentClass(this);
	return Result;
}

UProperty* UK2Node_Variable::GetPropertyForVariable() const
{
	const FName VarName = GetVarName();
	UEdGraphPin* VariablePin = FindPin(GetVarNameString());

	UProperty* VariableProperty = nullptr;

	// Need to look at parent Blueprint's skeleton classes to see if the variable property can resolve there.
	UClass* CurrentGeneratedClass = GetBlueprint()->GeneratedClass;
	while(CurrentGeneratedClass && VariableProperty == nullptr)
	{
		if(UBlueprint* CurrentBlueprint = Cast<UBlueprint>(CurrentGeneratedClass->ClassGeneratedBy))
		{
			VariableProperty = VariableReference.ResolveMember<UProperty>(CurrentBlueprint->SkeletonGeneratedClass);
			CurrentGeneratedClass = CurrentBlueprint->ParentClass;
		}
		else
		{
			break;
		}
	}

	// if the variable has been deprecated, don't use it
	if(VariableProperty != NULL)
	{
		if (VariableProperty->HasAllPropertyFlags(CPF_Deprecated))
		{
			VariableProperty = NULL;
		}
		// If the variable has been remapped update the pin
		else if (VariablePin && VarName != GetVarName())
		{
			VariablePin->PinName = GetVarNameString();
		}
	}

	return VariableProperty;
}

UEdGraphPin* UK2Node_Variable::GetValuePin() const
{
	UEdGraphPin* Pin = FindPin(GetVarNameString());
	check(Pin == NULL || Pin->Direction == EGPD_Output);
	return Pin;
}

void UK2Node_Variable::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	UProperty* VariableProperty = GetPropertyForVariable();

	// Local variables do not exist until much later in the compilation than this function can provide
	if (VariableProperty == NULL && !VariableReference.IsLocalScope())
	{
		if (!VariableReference.IsDeprecated())
		{
			FString OwnerName;

			UBlueprint* Blueprint = GetBlueprint();
			if (Blueprint != nullptr)
			{
				OwnerName = Blueprint->GetName();
				if (UClass* VarOwnerClass = VariableReference.GetMemberParentClass(Blueprint->GeneratedClass))
				{
					OwnerName = VarOwnerClass->GetName();
				}
			}
			FString const VarName = VariableReference.GetMemberName().ToString();

			FText const WarningFormat = LOCTEXT("VariableNotFound", "Could not find a variable named \"%s\" in '%s'.\nMake sure '%s' has been compiled for @@");
			MessageLog.Warning(*FString::Printf(*WarningFormat.ToString(), *VarName, *OwnerName, *OwnerName), this);
		}
		else
		{
			MessageLog.Warning(*FString::Printf(*LOCTEXT("VariableDeprecated", "Variable '%s' for @@ was deprecated.  Please update it.").ToString(), *VariableReference.GetMemberName().ToString()), this);
		}
	}

	if (VariableProperty && (VariableProperty->ArrayDim > 1))
	{
		MessageLog.Warning(*LOCTEXT("StaticArray_Warning", "@@ - the native property is a static array, which is not supported by blueprints").ToString(), this);
	}
}

FName UK2Node_Variable::GetPaletteIcon(FLinearColor& ColorOut) const
{
	FName ReturnIconName;

	if(VariableReference.IsLocalScope())
	{
		ReturnIconName = GetVariableIconAndColor(VariableReference.GetMemberScope(this), GetVarName(), ColorOut);
	}
	else
	{
		ReturnIconName = GetVariableIconAndColor(GetVariableSourceClass(), GetVarName(), ColorOut);
	}

	return ReturnIconName;
}

FName UK2Node_Variable::GetVarIconFromPinType(const FEdGraphPinType& InPinType, FLinearColor& IconColorOut)
{
	FName IconBrush = TEXT("Kismet.AllClasses.VariableIcon");

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	IconColorOut = K2Schema->GetPinTypeColor(InPinType);

	if(InPinType.bIsArray)
	{
		IconBrush = TEXT("Kismet.AllClasses.ArrayVariableIcon");
	}
	else if(InPinType.PinSubCategoryObject.IsValid())
	{
		if(UClass* Class = Cast<UClass>(InPinType.PinSubCategoryObject.Get()))
		{
			IconBrush = FClassIconFinder::FindIconNameForClass( Class );
		}
	}

	return IconBrush;
}

FText UK2Node_Variable::GetToolTipHeading() const
{
	FText Heading = Super::GetToolTipHeading();

	UProperty const* VariableProperty = VariableReference.ResolveMember<UProperty>(this);
	if (VariableProperty && VariableProperty->HasAllPropertyFlags(CPF_Net))
	{
		FText ReplicatedTag = LOCTEXT("ReplicatedVar", "Replicated");
		if (Heading.IsEmpty())
		{
			Heading = ReplicatedTag;
		}
		else 
		{
			Heading = FText::Format(FText::FromString("{0}\n{1}"), ReplicatedTag, Heading);
		}
	}

	return Heading;
}

void UK2Node_Variable::GetNodeAttributes( TArray<TKeyValuePair<FString, FString>>& OutNodeAttributes ) const
{
	UProperty* VariableProperty = GetPropertyForVariable();
	const FString VariableName = VariableProperty ? VariableProperty->GetName() : TEXT( "InvalidVariable" );
	OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "Type" ), TEXT( "Variable" ) ));
	OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "Class" ), GetClass()->GetName() ));
	OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "Name" ), VariableName ));
}

FName UK2Node_Variable::GetVariableIconAndColor(const UStruct* VarScope, FName VarName, FLinearColor& IconColorOut)
{
	FName IconBrush = TEXT("Kismet.AllClasses.VariableIcon");

	if(VarScope != NULL)
	{
		UProperty* Property = FindField<UProperty>(VarScope, VarName);
		if(Property != NULL)
		{
			const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

			FEdGraphPinType PinType;
			if(K2Schema->ConvertPropertyToPinType(Property,  PinType)) // use schema to get the color
			{
				IconBrush = GetVarIconFromPinType(PinType, IconColorOut);
			}
		}
	}

	return IconBrush;
}


void UK2Node_Variable::CheckForErrors(const UEdGraphSchema_K2* Schema, FCompilerResultsLog& MessageLog)
{
	if(!VariableReference.IsSelfContext() && VariableReference.GetMemberParentClass(this) != NULL)
	{
		// Check to see if we're not a self context, if we have a valid context.  It may have been purged because of a dead execution chain
		UEdGraphPin* ContextPin = Schema->FindSelfPin(*this, EGPD_Input);
		if((ContextPin != NULL) && (ContextPin->LinkedTo.Num() == 0) && (ContextPin->DefaultObject == NULL))
		{
			MessageLog.Error(*LOCTEXT("VarNodeError_InvalidVarTarget", "Variable node @@ uses an invalid target.  It may depend on a node that is not connected to the execution chain, and got purged.").ToString(), this);
		}
	}
}

void UK2Node_Variable::ReconstructNode()
{
	// update the variable reference if the property was renamed
	UClass* const VarClass = GetVariableSourceClass();
	if (VarClass)
	{
		bool bRemappedProperty = false;
		UClass* SearchClass = VarClass;
		while (SearchClass != NULL)
		{
			const TMap<FName, FName>* const ClassTaggedPropertyRedirects = UStruct::TaggedPropertyRedirects.Find( SearchClass->GetFName() );
			if (ClassTaggedPropertyRedirects)
			{
				const FName* const NewPropertyName = ClassTaggedPropertyRedirects->Find( VariableReference.GetMemberName() );
				if (NewPropertyName)
				{
					if (VariableReference.IsSelfContext())
					{
						VariableReference.SetSelfMember( *NewPropertyName );
					}
					else
					{
						VariableReference.SetExternalMember( *NewPropertyName, VarClass );
					}

					// found, can break
					bRemappedProperty = true;
					break;
				}
			}

			SearchClass = SearchClass->GetSuperClass();
		}

		if (!bRemappedProperty)
		{
			static FName OldVariableName(TEXT("UpdatedComponent"));
			static FName NewVariableName(TEXT("UpdatedPrimitive"));
			bRemappedProperty = RemapRestrictedLinkReference(OldVariableName, NewVariableName, UMovementComponent::StaticClass(), UPrimitiveComponent::StaticClass(), true);
		}
	}

	const FGuid VarGuid = VariableReference.GetMemberGuid();
	if (VarGuid.IsValid())
	{
		const FName VarName = UBlueprint::GetFieldNameFromClassByGuid<UProperty>(VarClass, VarGuid);
		if (VarName != NAME_None && VarName != VariableReference.GetMemberName())
		{
			if (VariableReference.IsSelfContext())
			{
				VariableReference.SetSelfMember( VarName );
			}
			else
			{
				VariableReference.SetExternalMember( VarName, VarClass );
			}
		}
	}

	Super::ReconstructNode();
}


bool UK2Node_Variable::RemapRestrictedLinkReference(FName OldVariableName, FName NewVariableName, const UClass* MatchInVariableClass, const UClass* RemapIfLinkedToClass, bool bLogWarning)
{
	bool bRemapped = false;
	if (VariableReference.GetMemberName() == OldVariableName)
	{
		UClass* const VarClass = GetVariableSourceClass();
		if (VarClass->IsChildOf(MatchInVariableClass))
		{
			UEdGraphPin* VariablePin = GetValuePin();
			if (VariablePin)
			{
				for (UEdGraphPin* OtherPin : VariablePin->LinkedTo)
				{
					if (OtherPin != nullptr && VariablePin->PinType.PinCategory == OtherPin->PinType.PinCategory)
					{
						// If any other pin we are linked to is a more restricted type, we need to do the remap.
						const UClass* OtherPinClass = Cast<UClass>(OtherPin->PinType.PinSubCategoryObject.Get());
						if (OtherPinClass && OtherPinClass->IsChildOf(RemapIfLinkedToClass))
						{
							if (VariableReference.IsSelfContext())
							{
								VariableReference.SetSelfMember(NewVariableName);
							}
							else
							{
								VariableReference.SetExternalMember(NewVariableName, VarClass);
							}
							bRemapped = true;
							break;
						}
					}
				}
			}
		}
	}

	if (bRemapped && bLogWarning && GetBlueprint())
	{
		FMessageLog("BlueprintLog").Warning(
			FText::Format(
			LOCTEXT("RemapRestrictedLinkReference", "{0}: Variable '{1}' was automatically changed to '{2}'. Verify that logic works as intended. (This warning will disappear once the blueprint has been resaved)"),
			FText::FromString(GetBlueprint()->GetPathName()),
			FText::FromString(MatchInVariableClass->GetName() + TEXT(".") + OldVariableName.ToString()),
			FText::FromString(MatchInVariableClass->GetName() + TEXT(".") + NewVariableName.ToString())
			));
	}

	return bRemapped;
}


FName UK2Node_Variable::GetCornerIcon() const
{
	const UProperty* VariableProperty = VariableReference.ResolveMember<UProperty>(this);
	if (VariableProperty && VariableProperty->HasAllPropertyFlags(CPF_Net))
	{
		return TEXT("Graph.Replication.Replicated");
	}

	return Super::GetCornerIcon();
}

bool UK2Node_Variable::HasExternalBlueprintDependencies(TArray<class UStruct*>* OptionalOutput) const
{
	UClass* SourceClass = GetVariableSourceClass();
	UBlueprint* SourceBlueprint = GetBlueprint();
	const bool bResult = (SourceClass && (SourceClass->ClassGeneratedBy != NULL) && (SourceClass->ClassGeneratedBy != SourceBlueprint));
	if (bResult && OptionalOutput)
	{
		OptionalOutput->Add(SourceClass);
	}
	return bResult || Super::HasExternalBlueprintDependencies(OptionalOutput);
}

FString UK2Node_Variable::GetDocumentationLink() const
{
	if( UProperty* Property = GetPropertyForVariable() )
	{
		// discover if the variable property is a non blueprint user variable
		UClass* SourceClass = Property->GetOwnerClass();
		if( SourceClass && SourceClass->ClassGeneratedBy == NULL )
		{
			UStruct* OwnerStruct = Property->GetOwnerStruct();

			if( OwnerStruct )
			{
				return FString::Printf( TEXT("Shared/Types/%s%s"), OwnerStruct->GetPrefixCPP(), *OwnerStruct->GetName() );
			}
		}
	}
	return TEXT( "" );
}

FString UK2Node_Variable::GetDocumentationExcerptName() const
{
	return GetVarName().ToString();
}

void UK2Node_Variable::AutowireNewNode(UEdGraphPin* FromPin)
{
	const UEdGraphSchema_K2* K2Schema = CastChecked<UEdGraphSchema_K2>(GetSchema());

	// Do some auto-connection
	if (FromPin != NULL)
	{
		bool bConnected = false;
		if(FromPin->Direction == EGPD_Output)
		{
			// If the source pin has a valid PinSubCategoryObject, we might be doing BP Comms, so check if it is a class
			if ((FromPin->PinType.PinSubCategoryObject.IsValid() && FromPin->PinType.PinSubCategoryObject->IsA(UClass::StaticClass())) || FromPin->PinType.PinSubCategory == K2Schema->PSC_Self)
			{
				UProperty* VariableProperty = GetPropertyForVariable();
				if(VariableProperty)
				{
					UClass* PropertyOwner = VariableProperty->GetOwnerClass();
					if (PropertyOwner != nullptr)
					{
						PropertyOwner = PropertyOwner->GetAuthoritativeClass();
					}

					// If the pin is a self reference, check if the UProperty is valid for the current Blueprint.
					const bool bIsSelfReferenceValid = (FromPin->PinType.PinSubCategory == K2Schema->PSC_Self)? GetBlueprint()->GeneratedClass->IsChildOf(PropertyOwner) : false;

					// BP Comms is highly likely at this point, if the source pin's type is a child of the variable's owner class, let's conform the "Target" pin
					if(FromPin->PinType.PinSubCategoryObject == PropertyOwner || dynamic_cast<UClass*>(FromPin->PinType.PinSubCategoryObject.Get())->IsChildOf(PropertyOwner) || bIsSelfReferenceValid)
					{
						UEdGraphPin* TargetPin = FindPin(K2Schema->PN_Self);
						TargetPin->PinType.PinSubCategoryObject = PropertyOwner;

						if(K2Schema->TryCreateConnection(FromPin, TargetPin))
						{
							bConnected = true;

							// Setup the VariableReference correctly since it may no longer be a self member
							VariableReference.SetFromField<UProperty>(GetPropertyForVariable(), false);
							TargetPin->bHidden = false;
							FromPin->GetOwningNode()->NodeConnectionListChanged();
							this->NodeConnectionListChanged();
						}
					}
				}
			}
		}

		if(!bConnected)
		{
			Super::AutowireNewNode(FromPin);
		}
	}
}

FBPVariableDescription const* UK2Node_Variable::GetBlueprintVarDescription() const
{
	FName const& VarName = VariableReference.GetMemberName();
	UStruct const* VariableScope = VariableReference.GetMemberScope(this);

	bool const bIsLocalVariable = (VariableScope != nullptr);
	if (bIsLocalVariable)
	{
		return FBlueprintEditorUtils::FindLocalVariable(GetBlueprint(), VariableScope, VarName);
	}
	else if (UProperty const* VarProperty = GetPropertyForVariable())
	{
		UClass const* SourceClass = VarProperty->GetOwnerClass();
		UBlueprint const* SourceBlueprint = (SourceClass != nullptr) ? Cast<UBlueprint>(SourceClass->ClassGeneratedBy) : nullptr;

		if (SourceBlueprint != nullptr)
		{
			int32 const VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(SourceBlueprint, VarName);
			return &SourceBlueprint->NewVariables[VarIndex];
		}
	}
	return nullptr;
}

bool UK2Node_Variable::CanPasteHere(const UEdGraph* TargetGraph) const
{
	// Do not allow pasting of variables in BPs that cannot handle them
	if ( FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph)->BlueprintType == BPTYPE_MacroLibrary && VariableReference.IsSelfContext() )
	{
		// Self variables must be from a parent class to the macro BP
		if(UProperty* Property = VariableReference.ResolveMember<UProperty>(this))
		{
			const UClass* CurrentClass = GetBlueprint()->SkeletonGeneratedClass->GetAuthoritativeClass();
			const UClass* PropertyClass = Property->GetOwnerClass()->GetAuthoritativeClass();
			const bool bIsChildOf = CurrentClass->IsChildOf(PropertyClass);
			return bIsChildOf;
		}
		return false;
	}
	return true;
}

#undef LOCTEXT_NAMESPACE
