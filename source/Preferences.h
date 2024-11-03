/*
 * Copyright 2024 Nexus6 <nexus6@disroot.org>
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */

#pragma once


#include <Messenger.h>
#include <String.h>


class Preferences
{
public:
						Preferences(BMessenger target);
						~Preferences();

	template <class T>
	void				SetProperty(BString property, T value);
	template <class T>
	T 					GetProperty(BString property, T defaultValue);
	template <class T>
	T 					GetPropertyWithIndex(BString property,  int32 index, T defaultValue);
	template <class T>
	void				CreateProperty(BString property, T param);
	template <class T>
	void				DeleteProperty(BString property, T param);

	int32				CountProperties(BString property);

	template <class T>
	T					GetTabProperty(int32 tabIndex, BString property, T defaultValue);
	template <class T>
	status_t			SetTabProperty(int32 tabIndex, BString property, T value);

private:
	BMessenger 			fTarget;
};

template <class T>
void
Preferences::SetProperty(BString property, T value)
{
	BMessage message(B_SET_PROPERTY);
	message.AddSpecifier(property);
	if constexpr (std::is_same<T, int32>::value) {
		message.AddInt32("data", value);
    } else if constexpr (std::is_same<T, bool>::value) {
		message.AddBool("data", value);
	} else if constexpr (std::is_same<T, rgb_color>::value) {
		message.AddData("data", B_RGB_COLOR_TYPE, &value, sizeof(value));
			// AddColor() can't work because it uses B_RGB_32_BIT
	}
	fTarget.SendMessage(&message);
}

template <class T>
T
Preferences::GetProperty(BString property, T defaultValue)
{
	BMessage message(B_GET_PROPERTY);
	message.AddSpecifier(property);
	BMessage reply;
	fTarget.SendMessage(&message, &reply);
	int32 result = reply.GetInt32("error", -1);
	if (result != -1) {
		T *data = nullptr;
		ssize_t numBytes = 0;
		status_t status = reply.FindData("response", B_ANY_TYPE, 0, (const void**)&data, &numBytes);
		if (status == B_OK)
			return *data;
	}
	return defaultValue;
}

template <class T>
T
Preferences::GetPropertyWithIndex(BString property, int32 index, T defaultValue)
{
	BMessage message(B_GET_PROPERTY);
	message.AddSpecifier(property, index);
	BMessage reply;
	fTarget.SendMessage(&message, &reply);
	int32 result = reply.GetInt32("error", -1);
	if (result != -1) {
		T *data = nullptr;
		ssize_t numBytes = 0;
		status_t status = reply.FindData("response", B_ANY_TYPE, 0, (const void**)&data, &numBytes);
		if (status == B_OK)
			return *data;
	}
	return defaultValue;
}

template <class T>
void
Preferences::CreateProperty(BString property, T param)
{
	BMessage message(B_CREATE_PROPERTY);
	message.AddSpecifier(property, param);
	fTarget.SendMessage(&message);
}

template <class T>
void
Preferences::DeleteProperty(BString property, T param)
{
	BMessage message(B_DELETE_PROPERTY);
	message.AddSpecifier(property, param);
	fTarget.SendMessage(&message);
}

template <class T>
T
Preferences::GetTabProperty(int32 tabIndex, BString property, T defaultValue)
{
	BMessage message(B_GET_PROPERTY);
	message.AddSpecifier(property);
	message.AddSpecifier("tab", tabIndex);
	BMessage reply;
	fTarget.SendMessage(&message, &reply);
	int32 result = reply.GetInt32("error", -1);
	if (result != -1) {
		if constexpr (std::is_same<T, BString>::value) {
			BString data;
			status_t status = reply.FindString("response", &data);
			if (status == B_OK)
				return data;
		} else {
			T *data = nullptr;
			ssize_t numBytes = 0;
			status_t status = reply.FindData("response", B_ANY_TYPE, 0, (const void**)&data, &numBytes);
			if (status == B_OK)
				return *data;
		}

	}
	return defaultValue;
}

template <class T>
status_t
Preferences::SetTabProperty(int32 tabIndex, BString property, T value)
{
	BMessage message(B_SET_PROPERTY);
	message.AddSpecifier(property);
	message.AddSpecifier("tab", tabIndex);
	if constexpr (std::is_same<T, int32>::value) {
		message.AddInt32("data", value);
    } else if constexpr (std::is_same<T, BString>::value) {
		message.AddString("data", value.String());
	} else if constexpr (std::is_same<T, rgb_color>::value) {
		message.AddData("data", B_RGB_COLOR_TYPE, &value, sizeof(value));
			// AddColor() can't work because it uses B_RGB_32_BIT
	}
	BMessage reply;
	fTarget.SendMessage(&message, &reply);
	return reply.GetInt32("error", -1);
}