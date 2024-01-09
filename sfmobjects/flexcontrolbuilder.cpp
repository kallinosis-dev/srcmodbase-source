//========= Copyright Valve Corporation, All rights reserved. ============//
//
// A class used to build flex animation controls for an animation set
//
//=============================================================================

#include "sfmobjects/flexcontrolbuilder.h"
#include "studio.h"
#include "movieobjects/dmeanimationset.h"
#include "movieobjects/dmeclip.h"
#include "movieobjects/dmetrackgroup.h"
#include "movieobjects/dmetrack.h"
#include "movieobjects/dmegamemodel.h"
#include "movieobjects/dmechannel.h"
//#include "movieobjects/dmebalancetostereocalculatoroperator.h"
#include "tier1/utlsymbol.h"

#if 0
// Names of attributes in controls we attach channels to
#define CONTROL_CHANNEL_ATTRIBUTE_COUNT 4
static const char *s_pChannelControls[CONTROL_CHANNEL_ATTRIBUTE_COUNT] =
{
	"channel", "valuechannel", "balancechannel", "multilevelchannel"
};


//-----------------------------------------------------------------------------
// Flex controller
//-----------------------------------------------------------------------------
class CDefaultGlobalFlexController : public IGlobalFlexController
{
public:
	CDefaultGlobalFlexController() : m_SymbolTable( 0, 32, true ) {}

	virtual int	FindGlobalFlexController( const char *name )
	{
		return m_SymbolTable.AddString( name );
	}

	virtual const char *GetGlobalFlexControllerName( int idx )
	{
		return m_SymbolTable.String( (CUtlSymbol)idx );
	}

private:
	CUtlSymbolTable m_SymbolTable;
};

static CDefaultGlobalFlexController s_GlobalFlexController;
extern IGlobalFlexController *g_pGlobalFlexController;


//-----------------------------------------------------------------------------
// This builds a list of the desired flex controllers we need to have controls for
// by the time we're all done with this enormous process.
//-----------------------------------------------------------------------------
void CFlexControlBuilder::BuildDesiredFlexControlList( CDmeGameModel *pGameModel )
{
	CStudioHdr cHdr( pGameModel->GetStudioHdr() );
	LocalFlexController_t nCount = cHdr.numflexcontrollers();

	m_FlexControllerInfo.EnsureCapacity( nCount );
	for ( LocalFlexController_t i = LocalFlexController_t(0); i < nCount; ++i )
	{
		int j = m_FlexControllerInfo.AddToTail();

		FlexControllerInfo_t& info = m_FlexControllerInfo[j];
		mstudioflexcontroller_t *pFlex = cHdr.pFlexcontroller( i );
		Q_strncpy( info.m_pFlexControlName, pFlex->pszName(), sizeof( info.m_pFlexControlName ) );
		info.m_nGlobalIndex = g_pGlobalFlexController->FindGlobalFlexController( pFlex->pszName() );
		info.m_flDefaultValue = 0.0f;
		if ( pFlex->max != pFlex->min )
		{
			// FIXME: Is this the correct default value?
			info.m_flDefaultValue = ( 0.0f - pFlex->min ) / ( pFlex->max - pFlex->min );
		}
	}
}


//-----------------------------------------------------------------------------
// This builds a list of the desired input controls we need to have controls for
// by the time we're all done with this enormous process.
//-----------------------------------------------------------------------------
void CFlexControlBuilder::BuildDesiredControlList( CDmeGameModel *pGameModel )
{
	int nCount = m_FlexControllerInfo.Count();
	for ( int i = 0; i < nCount; ++i )
	{
		int j = m_ControlInfo.AddToTail();
		ControlInfo_t &controlInfo = m_ControlInfo[j];
		memset( &controlInfo, 0, sizeof(ControlInfo_t) );

		FlexControllerInfo_t& info = m_FlexControllerInfo[i];
		const char *pFlexName = info.m_pFlexControlName;

		// Deal with stereo/mono controls
		if ( !Q_strnicmp( "right_", pFlexName, 6 ) && ( i < nCount - 1 ) )
		{
			FlexControllerInfo_t& leftInfo = m_FlexControllerInfo[i+1];
			Assert( !Q_strnicmp( "left_", leftInfo.m_pFlexControlName, 5 ) );

			controlInfo.m_bIsStereo = true;
			controlInfo.m_pControllerIndex[ OUTPUT_RIGHT ] = i;
			controlInfo.m_pControllerIndex[ OUTPUT_LEFT ] = i+1;
			Q_strncpy( controlInfo.m_pControlName, pFlexName + 6, sizeof(controlInfo.m_pControlName) );

			// Convert default values into value/balance
			LeftRightToValueBalance( &controlInfo.m_pDefaultValue[ CONTROL_VALUE ], 
				&controlInfo.m_pDefaultValue[ CONTROL_BALANCE ],
				leftInfo.m_flDefaultValue, info.m_flDefaultValue );

			// Skip the 'left_' flex control
			++i;
		}
		else
		{
			controlInfo.m_bIsStereo = false;
			controlInfo.m_pControllerIndex[ OUTPUT_MONO ] = i;
			controlInfo.m_pControllerIndex[ OUTPUT_LEFT ] = -1;
			Q_strncpy( controlInfo.m_pControlName, pFlexName, sizeof(controlInfo.m_pControlName) );
			controlInfo.m_pDefaultValue[ CONTROL_VALUE ] = info.m_flDefaultValue;
			controlInfo.m_pDefaultValue[ CONTROL_BALANCE ] = 0.5f;
		}

		// Deal with multi controls
		controlInfo.m_bIsMulti = ( i+1 < nCount ) && !Q_strnicmp( "multi_", m_FlexControllerInfo[ i+1 ].m_pFlexControlName, 6 );
		if ( controlInfo.m_bIsMulti )
		{
			FlexControllerInfo_t& multiInfo = m_FlexControllerInfo[i+1];

			controlInfo.m_pControllerIndex[ OUTPUT_MULTILEVEL ] = i+1;
			controlInfo.m_pDefaultValue[ CONTROL_MULTILEVEL ] = multiInfo.m_flDefaultValue;

			// Skip the 'multi_' flex control
			++i;
		}
		else
		{
			controlInfo.m_pControllerIndex[ OUTPUT_MULTILEVEL ] = -1;
			controlInfo.m_pDefaultValue[ CONTROL_MULTILEVEL ] = 0.5f;
		}
	}
}


//-----------------------------------------------------------------------------
// Finds a desired flex controller index in the m_FlexControllerInfo array
//-----------------------------------------------------------------------------
int CFlexControlBuilder::FindDesiredFlexController( const char *pFlexControllerName ) const
{
	int nCount = m_FlexControllerInfo.Count();
	for ( int i = 0; i < nCount; ++i )
	{
		if ( !Q_stricmp( pFlexControllerName, m_FlexControllerInfo[i].m_pFlexControlName ) )
			return i;
	}
	return -1;
}


//-----------------------------------------------------------------------------
// Removes a channel from the channels clip referring to it.
//-----------------------------------------------------------------------------
void CFlexControlBuilder::RemoveChannelFromClips( CDmeChannel *pChannel )
{
	// First, try to grab the channels referring to this op
	CUtlVector< CDmeChannelsClip* > channelsClips;
	FindAncestorsReferencingElement( pChannel, channelsClips );
	int nChannelsClips = channelsClips.Count();
	for ( int i = 0; i < nChannelsClips; ++i )
	{
		channelsClips[ i ]->RemoveChannel( pChannel );
	}

	// Next, remove the channel from values controls it may be attached to
	for (auto& s_pChannelControl : s_pChannelControls)
	{
		UtlSymId_t symChannelControl = g_pDataModel->GetSymbol(s_pChannelControl);
		CDmElement *pControl = FindReferringElement< CDmElement >( pChannel, symChannelControl );
		if ( pControl )
		{
			pControl->RemoveAttribute(s_pChannelControl);
		}
	}
}


//-----------------------------------------------------------------------------
// Blows away the various elements trying to control a flex controller op
//-----------------------------------------------------------------------------
void CFlexControlBuilder::CleanupExistingFlexController( CDmeGameModel *pGameModel, CDmeGlobalFlexControllerOperator *pOp )
{
	CDmeBalanceToStereoCalculatorOperator *pStereoOp;

	// First, try to grab the channel referring to this op
	const static UtlSymId_t symToElement = g_pDataModel->GetSymbol( "toElement" );
	CDmeChannel *pChannel = FindReferringElement< CDmeChannel >( pOp, symToElement );
	if ( !pChannel )
		goto destroyOp;

	// Sometimes a stereo op will be read from by this channel
	pStereoOp = CastElement< CDmeBalanceToStereoCalculatorOperator >( pChannel->GetFromElement() );
	RemoveChannelFromClips( pChannel );
	DestroyElement( pChannel );
	if ( !pStereoOp )
		goto destroyOp;

	RemoveStereoOpFromSet( pStereoOp );

	// If we have a stereo op, then blow away all channels targetting that stereo op
	DmAttributeReferenceIterator_t i = g_pDataModel->FirstAttributeReferencingElement( pStereoOp->GetHandle() );
	DmAttributeReferenceIterator_t next;
	for ( ; i != DMATTRIBUTE_REFERENCE_ITERATOR_INVALID; i = next )
	{
		next = g_pDataModel->NextAttributeReferencingElement( i );

		CDmAttribute *pAttribute = g_pDataModel->GetAttribute( i );
		pChannel = CastElement<CDmeChannel>( pAttribute->GetOwner() );
		if ( pChannel && pAttribute->GetNameSymbol() == symToElement )
		{
			RemoveChannelFromClips( pChannel );
			DestroyElement( pChannel );
		}
	}

	DestroyElement( pStereoOp );

destroyOp:
	pGameModel->RemoveGlobalFlexController( pOp );
	DestroyElement( pOp );
}

bool RemoveChannelIfUnused( CDmeChannel *pChannel, CDmeChannelsClip *pChannelsClip )
{
	if ( !pChannel )
		return false;
	if ( pChannel->GetToElement() != nullptr)
		return false;

	pChannelsClip->RemoveChannel( pChannel );
	DestroyElement( pChannel );
	return true;
}

// finds controls whose channels don't point to anything anymore, and deletes both the channels and the control
void CFlexControlBuilder::RemoveUnusedControlsAndChannels( CDmeAnimationSet *pAnimationSet, CDmeChannelsClip *pChannelsClip )
{
	CDmrElementArray<> controls = pAnimationSet->GetControls();
	int nControls = controls.Count();
	for ( int i = nControls - 1; i >= 0 ; --i )
	{
		CDmElement *pControl = controls[ i ];
		if ( pControl )
		{
			bool bRemoved =        RemoveChannelIfUnused( pControl->GetValueElement< CDmeChannel >( "channel" ), pChannelsClip );
			bRemoved = bRemoved || RemoveChannelIfUnused( pControl->GetValueElement< CDmeChannel >( "valuechannel" ), pChannelsClip );
			bRemoved = bRemoved || RemoveChannelIfUnused( pControl->GetValueElement< CDmeChannel >( "balancechannel" ), pChannelsClip );
			bRemoved = bRemoved || RemoveChannelIfUnused( pControl->GetValueElement< CDmeChannel >( "multilevelchannel" ), pChannelsClip );

			if ( !bRemoved )
				continue;

			DestroyElement( pControl );
		}

		controls.Remove( i );
	}
}

//-----------------------------------------------------------------------------
// This removes existing controls on the animationset that aren't in the desired state
//-----------------------------------------------------------------------------
void CFlexControlBuilder::RemoveUnusedExistingFlexControllers( CDmeGameModel *pGameModel )
{
	// These are the current flex controllers
	// NOTE: Name of these controllers should match the names of the flex controllers
	int nCount = pGameModel->NumGlobalFlexControllers();
	for ( int i = nCount; --i >= 0; )
	{
		CDmeGlobalFlexControllerOperator *pOp = pGameModel->GetGlobalFlexController( i );
		Assert( pOp );
		if ( pOp && FindDesiredFlexController( pOp->GetName() ) < 0 )
		{
			Msg( "removing flex controller %s\n", pOp->GetName() );

			CleanupExistingFlexController( pGameModel, pOp );
		}
	}
}


//-----------------------------------------------------------------------------
// Returns an existing mono log
//-----------------------------------------------------------------------------
void CFlexControlBuilder::GetExistingMonoLog( ExistingLogInfo_t *pExistingLog, 
											 CDmeFilmClip *pClip, CDmeGlobalFlexControllerOperator *pMonoOp )
{
	pExistingLog->m_pLog = nullptr;

	const static UtlSymId_t symToElement = g_pDataModel->GetSymbol( "toElement" );
	CDmeChannel *pMonoChannel = FindReferringElement< CDmeChannel >( pMonoOp, symToElement );
	if ( !pMonoChannel )
		return;

	// First, try to grab the channel referring to this op
	CDmeFloatLog *pLog = CastElement< CDmeFloatLog >( pMonoChannel->GetLog() );
	if ( !pLog )
		return;

	if ( ComputeChannelTimeTransform( &pExistingLog->m_GlobalOffset, &pExistingLog->m_flGlobalScale, pClip, pMonoChannel ) )
	{
		pExistingLog->m_pLog = pLog;
	}
}


//-----------------------------------------------------------------------------
// Finds a channels clip containing a particular channel
//-----------------------------------------------------------------------------
CDmeChannelsClip* CFlexControlBuilder::FindChannelsClipContainingChannel( CDmeFilmClip *pClip, CDmeChannel *pSearch )
{
	int gc = pClip->GetTrackGroupCount();
	for ( int i = 0; i < gc; ++i )
	{
		CDmeTrackGroup *pTrackGroup = pClip->GetTrackGroup( i );
		DMETRACKGROUP_FOREACH_CLIP_TYPE_START( CDmeChannelsClip, pTrackGroup, pTrack, pChannelsClip )

			int nChannels = pChannelsClip->m_Channels.Count();
		for ( int j = 0; j < nChannels; ++j )
		{
			CDmeChannel *pChannel = pChannelsClip->m_Channels[ j ];
			if ( pChannel == pSearch )
				return pChannelsClip;
		}

		DMETRACKGROUP_FOREACH_CLIP_TYPE_END()
	}
	return nullptr;
}


//-----------------------------------------------------------------------------
// Computes a global offset and scale to convert from log time to global time
//-----------------------------------------------------------------------------
void CFlexControlBuilder::ComputeChannelTimeTransform( DmeTime_t *pOffset, double *pScale, CDmeChannelsClip *pChannelsClip )
{
	// Determine the global time of the start + end of the log
	DmeClipStack_t srcStack;
	pChannelsClip->BuildClipStack( &srcStack, m_pMovie, nullptr);

	*pOffset = srcStack.FromChildMediaTime( DMETIME_ZERO, false );
	DmeTime_t duration = srcStack.FromChildMediaTime( DmeTime_t( 10000 ), false );
	duration -= *pOffset;
	*pScale = duration.GetSeconds();
}

bool CFlexControlBuilder::ComputeChannelTimeTransform( DmeTime_t *pOffset, double *pScale, CDmeFilmClip* pClip, CDmeChannel* pChannel )
{
	CDmeChannelsClip *pChannelsClip = FindChannelsClipContainingChannel( pClip, pChannel );
	if ( !pChannelsClip )
		return false;

	ComputeChannelTimeTransform( pOffset, pScale, pChannelsClip );
	return true;
}


static void AddKeyToLogs( CDmeTypedLog< float > *valueLog, CDmeTypedLog< float > *balanceLog, const DmeTime_t& keyTime, float lval, float rval )
{
#if 0
	// Convert left right into value, balance
	float value, balance;
	LeftRightToValueBalance( &value, &balance, lval, rval );

	//	Msg( "%.5f setting l/r %f %f to value %f balance %f\n",
	//		keyTime.GetSeconds(), lval, rval, value, balance );

	valueLog->SetKey( keyTime, value );
	balanceLog->SetKey( keyTime, balance );
#else
	AssertMsg(false, "Unimplemented because sfmobjects for CSGO were not leaked");
#endif
}

static void ConvertLRToVBLog( CDmeFloatLog *pValueLog, CDmeFloatLog *pBalanceLog, CDmeFloatLog *pLeftLog, CDmeFloatLog *pRightLog, DmeTime_t rightOffset, double flRightScale )
{
	int lc = pLeftLog->GetKeyCount();
	int rc = pRightLog->GetKeyCount();

	int nLeft = 0, nRight = 0;
	while ( nLeft < lc || nRight < rc )
	{
		bool bUseLeft = ( nLeft < lc );
		bool bUseRight = ( nRight < rc );

		DmeTime_t leftKeyTime = bUseLeft ? pLeftLog->GetKeyTime( nLeft ) : DMETIME_MAXTIME;
		DmeTime_t rightKeyTime = bUseRight ? pRightLog->GetKeyTime( nRight ) : DMETIME_MAXTIME;

		// Transform rightKeyTime into leftKeyTime space
		if ( bUseRight )
		{
			rightKeyTime.SetSeconds( rightKeyTime.GetSeconds() * flRightScale );
			rightKeyTime += rightOffset;
		}

		if ( leftKeyTime == rightKeyTime )
		{
			float lval = pLeftLog->GetKeyValue( nLeft++ );
			float rval = pRightLog->GetKeyValue( nRight++ );
			AddKeyToLogs( pValueLog, pBalanceLog, leftKeyTime, lval, rval );
			continue;
		}

		if ( leftKeyTime < rightKeyTime )
		{
			// pull a value from the right log at the leftKeyTime 
			// and advance to the next sample on the left side
			float lval = pLeftLog->GetKeyValue( nLeft++ );
			float rval = pRightLog->GetValue( leftKeyTime );
			AddKeyToLogs( pValueLog, pBalanceLog, leftKeyTime, lval, rval );
			continue;
		}

		// Pull a value from the left log at the rightKeyTime 
		// and advance to the next sample on the right side
		float lval = pLeftLog->GetValue( rightKeyTime );
		float rval = pRightLog->GetKeyValue( nRight++ );
		AddKeyToLogs( pValueLog, pBalanceLog, rightKeyTime, lval, rval );
	}
}


//-----------------------------------------------------------------------------
// Fixup list of existing flex controller logs
// - reattach flex controls that were removed from the gamemodel's list
//-----------------------------------------------------------------------------
void CFlexControlBuilder::FixupExistingFlexControlLogList( CDmeFilmClip *pCurrentClip, CDmeGameModel *pGameModel )
{
	int nTrackGroups = pCurrentClip->GetTrackGroupCount();
	for ( int gi = 0; gi < nTrackGroups; ++gi )
	{
		CDmeTrackGroup *pTrackGroup = pCurrentClip->GetTrackGroup( gi );
		if ( !pTrackGroup )
			continue;

		DMETRACKGROUP_FOREACH_CLIP_TYPE_START( CDmeChannelsClip, pTrackGroup, pTrack, pChannelsClip )
			int nChannels = pChannelsClip->m_Channels.Count();
			for ( int ci = 0; ci < nChannels; ++ci )
			{
				CDmeChannel *pChannel = pChannelsClip->m_Channels[ ci ];
				if ( !pChannel )
					continue;

				CDmeGlobalFlexControllerOperator *pOp = CastElement< CDmeGlobalFlexControllerOperator >( pChannel->GetToElement() );
				if ( !pOp )
					continue;

				if ( pOp->m_gameModel.GetHandle() != pGameModel->GetHandle() )
					continue;

				int nGlobalIndex = pOp->GetGlobalIndex();
				CDmeGlobalFlexControllerOperator *pFoundOp = pGameModel->FindGlobalFlexController( nGlobalIndex );
				if ( pFoundOp == pOp )
					continue;

				if ( !pFoundOp )
				{
					Msg( "adding missing flex controller %d %s\n", nGlobalIndex, pOp->GetName() );
					pFoundOp = pGameModel->AddGlobalFlexController( pOp->GetName(), nGlobalIndex );
				}
				pChannel->SetOutput( pFoundOp, pChannel->GetToAttribute()->GetName() );
				if ( pChannel->GetFromElement() == pOp )
				{
					pChannel->SetInput( pFoundOp, pChannel->GetFromAttribute()->GetName() );
				}
				Msg( "removing duplicate flex controller %d %s\n", nGlobalIndex, pOp->GetName() );
				RemoveElementFromRefereringAttributes( pOp );
				DestroyElement( pOp	);
			}
		DMETRACKGROUP_FOREACH_CLIP_TYPE_END();
	}
}

//-----------------------------------------------------------------------------
// Build list of existing flex controller logs
//-----------------------------------------------------------------------------
void CFlexControlBuilder::BuildExistingFlexControlLogList( CDmeFilmClip *pCurrentClip, CDmeGameModel *pGameModel )
{
	// These are the current flex controllers that also exist in the desired list
	// NOTE: Name of these controllers should match the names of the flex controllers
	int nCount = m_ControlInfo.Count();
	for ( int i = 0; i < nCount; ++i )
	{
		ControlInfo_t &info = m_ControlInfo[i];

		if ( info.m_bIsStereo )
		{
			int nRightFlex = info.m_pControllerIndex[ OUTPUT_RIGHT ];
			int nLeftFlex = info.m_pControllerIndex[ OUTPUT_LEFT ];
			FlexControllerInfo_t *pRightInfo = &m_FlexControllerInfo[nRightFlex];
			FlexControllerInfo_t *pLeftInfo = &m_FlexControllerInfo[nLeftFlex];

			CDmeGlobalFlexControllerOperator *pRightOp = pGameModel->FindGlobalFlexController( pRightInfo->m_nGlobalIndex );
			CDmeGlobalFlexControllerOperator *pLeftOp = pGameModel->FindGlobalFlexController( pLeftInfo->m_nGlobalIndex );
			if ( pRightOp && pLeftOp )
			{
				Msg( "replacing stereo flex controllers %s and %s\n", pRightOp->GetName(), pRightOp->GetName() );

				GetExistingStereoLog( info.m_pExistingLog, pCurrentClip, pRightOp, pLeftOp );
				CleanupExistingFlexController( pGameModel, pRightOp );
				CleanupExistingFlexController( pGameModel, pLeftOp );
			}
		}
		else
		{
			int nFlex = info.m_pControllerIndex[ OUTPUT_MONO ];
			FlexControllerInfo_t *pInfo = &m_FlexControllerInfo[nFlex];

			CDmeGlobalFlexControllerOperator *pMonoOp = pGameModel->FindGlobalFlexController( pInfo->m_nGlobalIndex );
			if ( pMonoOp )
			{
				Msg( "replacing mono flex controller %s\n", pMonoOp->GetName() );

				GetExistingMonoLog( &info.m_pExistingLog[CONTROL_VALUE], pCurrentClip, pMonoOp );
				CleanupExistingFlexController( pGameModel, pMonoOp );
			}
		}

		if ( info.m_bIsMulti )
		{
			int nFlex = info.m_pControllerIndex[ OUTPUT_MULTILEVEL ];
			FlexControllerInfo_t *pMultiInfo = &m_FlexControllerInfo[ nFlex ];
			CDmeGlobalFlexControllerOperator *pMultiOp = pGameModel->FindGlobalFlexController( pMultiInfo->m_nGlobalIndex );
			if ( pMultiOp )
			{
				Msg( "replacing multi flex controller %s\n", pMultiOp->GetName() );

				GetExistingMonoLog( &info.m_pExistingLog[CONTROL_MULTILEVEL], pCurrentClip, pMultiOp );
				CleanupExistingFlexController( pGameModel, pMultiOp );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Creates a flex controller and a channel connecting it to a control
//-----------------------------------------------------------------------------
struct FlexOpInfo_t
{
	const char *m_pControlAttributeName;
	const char *m_pControlLinkAttributeName;
};

static FlexOpInfo_t s_pFlexOpInfo[2] = 
{
	{ "value",		"" },
	{ "multilevel", "multilevel" },
};

void CFlexControlBuilder::BuildFlexControllerOps( CDmeGameModel *pGameModel, CDmeChannelsClip *pChannelsClip, ControlInfo_t &info, ControlField_t field )
{
	const FlexOpInfo_t& flexInfo = s_pFlexOpInfo[ ( field == CONTROL_VALUE ) ? 0 : 1 ];

	// Get the global flex controller name and index
	const FlexControllerInfo_t& fcInfo = m_FlexControllerInfo[ info.m_pControllerIndex[field] ];

	// Create operator which drives facial flex setting
	CDmeGlobalFlexControllerOperator *pFlexControllerOp = pGameModel->AddGlobalFlexController( 
		fcInfo.m_pFlexControlName, fcInfo.m_nGlobalIndex );

	// Create a channel which passes from the control value to the global flex controller
	char pName[ 256 ];
	Q_snprintf( pName, sizeof( pName ), "%s_flex_channel", fcInfo.m_pFlexControlName );
	info.m_ppControlChannel[field] = pChannelsClip->CreatePassThruConnection( pName, 
		info.m_pControl, flexInfo.m_pControlAttributeName, pFlexControllerOp, "flexWeight" );

	// NOTE: The animation set slider panel looks for these custom attributes
	Q_snprintf( pName, sizeof(pName), "%schannel", flexInfo.m_pControlLinkAttributeName );
	info.m_pControl->SetValue( pName, info.m_ppControlChannel[field] );

	// Switch the channel into play mode by default
	info.m_ppControlChannel[field]->SetMode( CM_PLAY );
}


//-----------------------------------------------------------------------------
// Creates a flex controller and a channel connecting it to stereo controls
//-----------------------------------------------------------------------------
static const char *s_pStereoOutputPrefix[2] = 
{
	"right",
	"left",
};

static const char *s_pStereoInputPrefix[2] = 
{
	"value",
	"balance",
};

//-----------------------------------------------------------------------------
// Build the infrastructure of the ops that connect that control to the dmegamemodel
//-----------------------------------------------------------------------------
void CFlexControlBuilder::AttachControlsToGameModel( CDmeAnimationSet *pAnimationSet,
													CDmeGameModel *pGameModel, CDmeChannelsClip *pChannelsClip )
{
	// Build the infrastructure of the ops that connect that control to the dmegamemodel
	int c = m_ControlInfo.Count();
	for ( int i = 0; i < c; ++i )
	{
		ControlInfo_t &info = m_ControlInfo[i];
		if ( info.m_bIsStereo )
		{
			BuildStereoFlexControllerOps( pAnimationSet, pGameModel, pChannelsClip, info );
		}
		else
		{
			BuildFlexControllerOps( pGameModel, pChannelsClip, info, CONTROL_VALUE );
		}

		if ( info.m_bIsMulti )
		{
			BuildFlexControllerOps( pGameModel, pChannelsClip, info, CONTROL_MULTILEVEL );
		}
	}
}


//-----------------------------------------------------------------------------
// Initializes the fields of a flex control
//-----------------------------------------------------------------------------
void CFlexControlBuilder::InitializeFlexControl( ControlInfo_t &info )
{
	CDmElement *pControl = info.m_pControl;

	// Remove these, if they exist...
	for ( int i = 0; i < CONTROL_CHANNEL_ATTRIBUTE_COUNT; ++i )
	{
		pControl->RemoveAttribute( s_pChannelControls[i] );
	}

	// Force these to always be up-to-date
	pControl->SetValue< bool >( "combo", info.m_bIsStereo );
	pControl->SetValue< bool >( "multi", info.m_bIsMulti );
	pControl->SetValue< float >( "defaultValue", info.m_pDefaultValue[CONTROL_VALUE] );
	pControl->SetValue< float >( "defaultBalance", info.m_pDefaultValue[CONTROL_BALANCE] );
	pControl->SetValue< float >( "defaultMultilevel", info.m_pDefaultValue[CONTROL_MULTILEVEL] );

	// These can keep their value if they already exist
	pControl->InitValue< float >( "value", info.m_pDefaultValue[CONTROL_VALUE] );
	if ( info.m_bIsStereo )
	{
		pControl->InitValue< float >( "balance", info.m_pDefaultValue[CONTROL_BALANCE] );
	}
	else
	{
		pControl->RemoveAttribute( "balance" );
	}
	if ( info.m_bIsMulti )
	{
		pControl->InitValue< float >( "multilevel", info.m_pDefaultValue[CONTROL_MULTILEVEL] );
	}
	else
	{
		pControl->RemoveAttribute( "multilevel" );
	}
}


//-----------------------------------------------------------------------------
// Creates all controls for flexes
//-----------------------------------------------------------------------------
void CFlexControlBuilder::CreateFlexControls( CDmeAnimationSet *pAnimationSet )
{
	// Create a facial control for all input controls
	int c = m_ControlInfo.Count();
	for ( int i = 0; i < c; ++i )
	{
		ControlInfo_t &info = m_ControlInfo[i];

		// Check to see if the animation set already has the control
		info.m_pControl = pAnimationSet->FindOrAddControl( info.m_pControlName );

		// Now initialize the fields of the flex control
		InitializeFlexControl( info );
	}
}


//-----------------------------------------------------------------------------
// Attaches existing logs and sets default values for logs
//-----------------------------------------------------------------------------
void CFlexControlBuilder::SetupLogs( CDmeChannelsClip *pChannelsClip, bool bUseExistingLogs )
{
	DmeTime_t targetOffset;
	double flTargetScale;
	ComputeChannelTimeTransform( &targetOffset, &flTargetScale, pChannelsClip );
	double flOOTargetScale = ( flTargetScale != 0.0 ) ? 1.0 / flTargetScale : 1.0;

	// Build the infrastructure of the ops that connect that control to the dmegamemodel
	int c = m_ControlInfo.Count();
	for ( int i = 0; i < c; ++i )
	{
		ControlInfo_t &info = m_ControlInfo[i];
		for ( int j = 0; j < CONTROL_FIELD_COUNT; ++j )
		{
			// Can happen for non-multi or non-stereo controls
			if ( !info.m_ppControlChannel[j] )
				continue;

			// Replace the existing log if we need to
			CDmeFloatLog *pFloatLog = CastElement< CDmeFloatLog >( info.m_ppControlChannel[j]->GetLog() );
			if ( bUseExistingLogs && info.m_pExistingLog[j].m_pLog )
			{
				info.m_ppControlChannel[j]->SetLog( info.m_pExistingLog[j].m_pLog );
				DestroyElement( pFloatLog );
				pFloatLog = info.m_pExistingLog[j].m_pLog;

				// Apply transform to get the log into the space of the current channel
				double flTotalScale = info.m_pExistingLog[j].m_flGlobalScale * flOOTargetScale;
				DmeTime_t totalOffset = info.m_pExistingLog[j].m_GlobalOffset - targetOffset;
				totalOffset.SetSeconds( totalOffset.GetSeconds() * flOOTargetScale );
				pFloatLog->ScaleBiasKeyTimes( flTotalScale, totalOffset );
			}

			// Set the default value for this log
			pFloatLog->SetDefaultValue( info.m_pDefaultValue[j] );
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Main entry point for creating flex animation set controls
//-----------------------------------------------------------------------------
void CFlexControlBuilder::CreateAnimationSetControls( CDmeFilmClip *pMovie, CDmeAnimationSet *pAnimationSet, 
	CDmeGameModel *pGameModel, CDmeFilmClip *pSourceClip, CDmeChannelsClip *pDestClip, bool bUseExistingLogs )
{
#if 0
	m_pMovie = pMovie;

	FixupExistingFlexControlLogList( pSourceClip, pGameModel );

	// First, look at the current mdl and determine what are its low-level flexcontrollers
	// [these are the outputs eventually driven by the animation set controls]
	BuildDesiredFlexControlList( pGameModel );

	// Next, based on the list of low-level flexcontrollers, determine a high-level set of input controls
	BuildDesiredControlList( pGameModel );

	// Next look at what the animation set currently thinks are the input controls + low-level flexcontrollers
	// and remove the unused ones
	RemoveUnusedExistingFlexControllers( pGameModel );

	RemoveUnusedControlsAndChannels( pAnimationSet, pDestClip );

	if ( bUseExistingLogs )
	{
		// Look at the current input controls + low-level flexcontrollers
		// and grab logs that drive them so we can apply them to the new controls
		BuildExistingFlexControlLogList( pSourceClip, pGameModel );
	}

	// Create the input controls we decided we needed in BuildDesiredControlList
	CreateFlexControls( pAnimationSet );

	// Build channels + control logis attaching the input controls to the low level flex controls
	AttachControlsToGameModel( pAnimationSet, pGameModel, pDestClip );

	// Attach existing logs to the new input controls created in CreateFlexControls
	SetupLogs( pDestClip, bUseExistingLogs );
#else
	AssertMsg(false, "Unimplemented because sfmobjects for CSGO were not leaked");
#endif
}


//-----------------------------------------------------------------------------
// Initialize default global flex controller
//-----------------------------------------------------------------------------
void SetupDefaultFlexController()
{
#if 0
	g_pGlobalFlexController = &s_GlobalFlexController;
#else
	AssertMsg(false, "Unimplemented because sfmobjects for CSGO were not leaked");
#endif
}