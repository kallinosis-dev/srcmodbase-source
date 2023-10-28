//======== Copyright © 2011, Valve Corporation, All rights reserved. ========//
//
// Purpose: Helper functions for protobufs
//
//===========================================================================//


#include "absl/log/globals.h"
#include "absl/log/log_sink.h"
#include "absl/log/log_sink_registry.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/reflection_ops.h"
#include "google/protobuf/descriptor.pb.h"

#include "tier0/dbg.h"

using namespace google::protobuf;


class AbseilLogSink: public absl::LogSink
{
public:
	void Send(const absl::LogEntry& entry) override
	{
		switch (entry.log_severity())
		{
		case absl::LogSeverity::kInfo:
		case absl::LogSeverity::kWarning:
			DevMsg("Protobuf: %s(%d): %s\n", entry.source_filename().data(), entry.source_line(), entry.text_message_with_prefix().data());
			break;

		case absl::LogSeverity::kError:
			Warning("Protobuf: %s(%d): %s\n", entry.source_filename().data(), entry.source_line(), entry.text_message_with_prefix().data());
			break;

		case absl::LogSeverity::kFatal:
			Warning("Protobuf: %s(%d): %s\n", entry.source_filename().data(), entry.source_line(), entry.text_message_with_prefix().data());
			Plat_ExitProcess(100);
			break;
		}
	}
};

class AbseilAddLogSink
{
	AbseilLogSink _sink;

public:
	AbseilAddLogSink()
	{
		absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfinity);
		absl::AddLogSink(&_sink);
	}

	~AbseilAddLogSink()
	{
		absl::RemoveLogSink(&_sink);
	}
};

static AbseilAddLogSink g_AbseilAddLogSink;

/*

//----------------------------------------------------------------------------
// Purpose: Copies a field into a third message if its different in the other messages
//----------------------------------------------------------------------------
bool ProtoBufDeltaMerge( const ::google::protobuf::Message &src, const ::google::protobuf::Message &delta, ::google::protobuf::Message* to ) 
{
	const Descriptor* descriptor = src.GetDescriptor();

	if ( descriptor != delta.GetDescriptor() )
	{
		Warning( "Tried to merge delta to a src message of a different type.");
		return false;
	}

	if ( descriptor != to->GetDescriptor() )
	{
		Warning( "Tried to merge dleta to a message of a different type.");
		return false;
	}

	to->Clear();

	const Reflection* src_reflection = src.GetReflection();
	const Reflection* delta_reflection = delta.GetReflection();
	const Reflection* to_reflection = to->GetReflection();

	Assert( src_reflection == delta_reflection );
	Assert( src_reflection == to_reflection );

	vector<const FieldDescriptor*> fields;
	src_reflection->ListFields(src, &fields);

	for ( size_t i = 0; i < fields.size(); i++ )
	{
		const FieldDescriptor* field = fields[i];

		// We can't really delta repeated fields, it sucks
		if ( field->is_repeated() )
		{
			int count = delta_reflection->FieldSize(delta, field);
			to_reflection->ClearField( to, field );

			for ( int j = 0; j < count; j++ )
			{
				switch ( field->cpp_type() )
				{
#define HANDLE_TYPE(CPPTYPE, METHOD) \
          case FieldDescriptor::CPPTYPE_##CPPTYPE: \
            to_reflection->Add##METHOD(to, field, delta_reflection->GetRepeated##METHOD(delta, field, j)); \
            break;

				HANDLE_TYPE(INT32 , Int32 );
				HANDLE_TYPE(INT64 , Int64 );
				HANDLE_TYPE(UINT32, UInt32);
				HANDLE_TYPE(UINT64, UInt64);
				HANDLE_TYPE(FLOAT , Float );
				HANDLE_TYPE(DOUBLE, Double);
				HANDLE_TYPE(BOOL  , Bool  );
				HANDLE_TYPE(STRING, String);
				HANDLE_TYPE(ENUM  , Enum  );
#undef HANDLE_TYPE

				case FieldDescriptor::CPPTYPE_MESSAGE:
					// We can recurse this!
					Message *to_element = to_reflection->AddMessage( to, field );
					if ( !ProtoBufDeltaMerge( src_reflection->GetRepeatedMessage( src, field, j ), delta_reflection->GetRepeatedMessage( delta, field, j ), to_element ) )
						return false;
					break;
				}
			}
		}
		else
		{
			switch ( field->cpp_type() )
			{
#define HANDLE_TYPE(CPPTYPE, METHOD) \
			case FieldDescriptor::CPPTYPE_##CPPTYPE: \
				if ( delta_reflection->HasField( delta, field ) ) \
				{ \
					to_reflection->Set##METHOD( to, field, delta_reflection->Get##METHOD( delta, field ) ); \
				} \
				else \
				{ \
					if ( src_reflection->HasField( src, field ) ) \
					{ \
						to_reflection->Set##METHOD( to, field, src_reflection->Get##METHOD( delta, field ) ); \
					} \
					else \
					{ \
						to_reflection->ClearField( to, field ); \
					} \
				} \
				break;

			HANDLE_TYPE(INT32 , Int32 );
			HANDLE_TYPE(INT64 , Int64 );
			HANDLE_TYPE(UINT32, UInt32);
			HANDLE_TYPE(UINT64, UInt64);
			HANDLE_TYPE(FLOAT , Float );
			HANDLE_TYPE(DOUBLE, Double);
			HANDLE_TYPE(BOOL  , Bool  );
			HANDLE_TYPE(STRING, String);
			HANDLE_TYPE(ENUM  , Enum  );
#undef HANDLE_TYPE

			case FieldDescriptor::CPPTYPE_MESSAGE:
				Message *to_element = to_reflection->MutableMessage( to, field );
				if ( delta_reflection->HasField( delta, field ) )
				{
					if ( !ProtoBufDeltaMerge( src_reflection->GetMessage( src, field ), delta_reflection->GetMessage( delta, field ), to_element ) )
						return false;
				}
				else
				{
					if ( src_reflection->HasField( src, field ) ) \
					{
						to_element->Clear();
						to_element->MergeFrom( src_reflection->GetMessage( src, field ) );
					}
					else
					{
						to_reflection->ClearField( to, field );
					}
				}
				break;
			}
		}
	}

	return true;
}

bool ProtoBufCalcDelta( const ::google::protobuf::Message &src, const ::google::protobuf::Message &dest, ::google::protobuf::Message *delta )
{
	const Descriptor* descriptor = src.GetDescriptor();

	if ( descriptor != dest.GetDescriptor() )
	{
		Warning( "Tried to calc delta to a src message of a different type.");
		return false;
	}

	if ( descriptor != delta->GetDescriptor() )
	{
		Warning( "Tried to calc delta to a message of a different type.");
		return false;
	}

	delta->Clear();

	const Reflection* src_reflection = src.GetReflection();
	const Reflection* dest_reflection = dest.GetReflection();
	const Reflection* delta_reflection = delta->GetReflection();

	Assert( src_reflection == dest_reflection );
	Assert( src_reflection == delta_reflection );

	vector<const FieldDescriptor*> fields;
	src_reflection->ListFields(src, &fields);

	for ( size_t i = 0; i < fields.size(); i++ )
	{
		const FieldDescriptor* field = fields[i];

		// We can't really delta repeated fields, it sucks
		if ( field->is_repeated() )
		{
			int count = dest_reflection->FieldSize(dest, field);
			delta_reflection->ClearField( delta, field );

			for ( int j = 0; j < count; j++ )
			{
				switch ( field->cpp_type() )
				{
#define HANDLE_TYPE(CPPTYPE, METHOD) \
		  case FieldDescriptor::CPPTYPE_##CPPTYPE: \
			delta_reflection->Add##METHOD(delta, field, dest_reflection->GetRepeated##METHOD(dest, field, j)); \
			break;

				HANDLE_TYPE(INT32 , Int32 );
				HANDLE_TYPE(INT64 , Int64 );
				HANDLE_TYPE(UINT32, UInt32);
				HANDLE_TYPE(UINT64, UInt64);
				HANDLE_TYPE(FLOAT , Float );
				HANDLE_TYPE(DOUBLE, Double);
				HANDLE_TYPE(BOOL  , Bool  );
				HANDLE_TYPE(STRING, String);
				HANDLE_TYPE(ENUM  , Enum  );
#undef HANDLE_TYPE

				case FieldDescriptor::CPPTYPE_MESSAGE:
					// We can recurse this!
					Message *delta_element = delta_reflection->AddMessage( delta, field );
					if ( !ProtoBufCalcDelta( src_reflection->GetRepeatedMessage( src, field, j ), dest_reflection->GetRepeatedMessage( dest, field, j ), delta_element ) )
						return false;
					break;
				}
			}
		}
		else
		{
			switch ( field->cpp_type() )
			{
#define HANDLE_TYPE(CPPTYPE, METHOD) \
			case FieldDescriptor::CPPTYPE_##CPPTYPE: \
				if ( dest_reflection->HasField( dest, field ) ) \
				{ \
					if ( !src_reflection->HasField( src, field ) || \
						 src_reflection->Get##METHOD( src, field ) != dest_reflection->Get##METHOD( dest, field ) ) \
					{ \
						delta_reflection->Set##METHOD( delta, field, dest_reflection->Get##METHOD( dest, field ) ); \
					} \
				} \
				else \
				{ \
					if ( src_reflection->HasField( src, field ) ) \
					{ \
						delta_reflection->Set##METHOD( delta, field, dest_reflection->Get##METHOD( dest, field ) ); \
					} \
				} \
				break;
			HANDLE_TYPE(INT32 , Int32 );
			HANDLE_TYPE(INT64 , Int64 );
			HANDLE_TYPE(UINT32, UInt32);
			HANDLE_TYPE(UINT64, UInt64);
			HANDLE_TYPE(FLOAT , Float );
			HANDLE_TYPE(DOUBLE, Double);
			HANDLE_TYPE(BOOL  , Bool  );
			HANDLE_TYPE(STRING, String);
			HANDLE_TYPE(ENUM  , Enum  );
#undef HANDLE_TYPE

			case FieldDescriptor::CPPTYPE_MESSAGE:
				Message *delta_element = delta_reflection->MutableMessage( delta, field );
				if ( !ProtoBufCalcDelta( src_reflection->GetMessage( src, field ), dest_reflection->GetMessage( dest, field ), delta_element ) )
					return false;
				break;
			}
		}
	}

	return true;
}
*/

