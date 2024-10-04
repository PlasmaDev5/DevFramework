#include <Foundation/FoundationPCH.h>

#if (PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP) && PL_ENABLED(PL_SUPPORTS_DIRECTORY_WATCHER))

#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <Foundation/Configuration/CVar.h>
#  include <Foundation/Containers/DynamicArray.h>
#  include <Foundation/IO/DirectoryWatcher.h>
#  include <Foundation/IO/Implementation/FileSystemMirror.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Win/DosDevicePath_Win.h>

// Comment in to get verbose output on the function of the directory watcher
// #define DEBUG_FILE_WATCHER

#  ifdef DEBUG_FILE_WATCHER
#    define DEBUG_LOG(...) plLog::Warning(__VA_ARGS__)
#  else
#    define DEBUG_LOG(...)
#  endif

namespace
{
  plCVarBool cvar_ForceNonNTFS("DirectoryWatcher.ForceNonNTFS", false, plCVarFlags::Default, "Forces the use of ReadDirectoryChanges instead of ReadDirectoryChangesEx");

  struct MoveEvent
  {
    plString path;
    bool isDirectory = false;

    void Clear()
    {
      path.Clear();
    }

    bool IsEmpty()
    {
      return path.IsEmpty();
    }
  };

  struct Change
  {
    plStringBuilder eventFilePath;
    bool isFile;
    DWORD Action;
    LARGE_INTEGER LastModificationTime;
  };

  using plFileSystemMirrorType = plFileSystemMirror<bool>;

  void GetChangesNTFS(plStringView sDirectoryPath, const plHybridArray<plUInt8, 4096>& buffer, plDynamicArray<Change>& ref_changes)
  {
    plUInt32 uiChanges = 1;
    auto info = (const FILE_NOTIFY_EXTENDED_INFORMATION*)buffer.GetData();
    while (info->NextEntryOffset != 0)
    {
      uiChanges++;
      info = (const FILE_NOTIFY_EXTENDED_INFORMATION*)(((plUInt8*)info) + info->NextEntryOffset);
    }
    ref_changes.Reserve(uiChanges);
    info = (const FILE_NOTIFY_EXTENDED_INFORMATION*)buffer.GetData();

    while (true)
    {
      auto directory = plArrayPtr<const WCHAR>(info->FileName, info->FileNameLength / sizeof(WCHAR));
      int bytesNeeded = WideCharToMultiByte(CP_UTF8, 0, directory.GetPtr(), directory.GetCount(), nullptr, 0, nullptr, nullptr);
      if (bytesNeeded > 0)
      {
        plHybridArray<char, 1024> dir;
        dir.SetCountUninitialized(bytesNeeded);
        WideCharToMultiByte(CP_UTF8, 0, directory.GetPtr(), directory.GetCount(), dir.GetData(), dir.GetCount(), nullptr, nullptr);

        Change& currentChange = ref_changes.ExpandAndGetRef();
        currentChange.eventFilePath = sDirectoryPath;
        currentChange.eventFilePath.AppendPath(plStringView(dir.GetData(), dir.GetCount()));
        currentChange.eventFilePath.MakeCleanPath();
        currentChange.Action = info->Action;
        currentChange.LastModificationTime = info->LastModificationTime;
        currentChange.isFile = (info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
      }

      if (info->NextEntryOffset == 0)
      {
        break;
      }
      else
        info = (const FILE_NOTIFY_EXTENDED_INFORMATION*)(((plUInt8*)info) + info->NextEntryOffset);
    }
  }

  void GetChangesNonNTFS(plStringView sDirectoryPath, const plHybridArray<plUInt8, 4096>& buffer, plDynamicArray<Change>& ref_changes)
  {
    plUInt32 uiChanges = 1;
    auto info = (const FILE_NOTIFY_INFORMATION*)buffer.GetData();
    while (info->NextEntryOffset != 0)
    {
      uiChanges++;
      info = (const FILE_NOTIFY_INFORMATION*)(((plUInt8*)info) + info->NextEntryOffset);
    }
    ref_changes.Reserve(ref_changes.GetCount() + uiChanges);
    info = (const FILE_NOTIFY_INFORMATION*)buffer.GetData();

    while (true)
    {
      auto directory = plArrayPtr<const WCHAR>(info->FileName, info->FileNameLength / sizeof(WCHAR));
      int bytesNeeded = WideCharToMultiByte(CP_UTF8, 0, directory.GetPtr(), directory.GetCount(), nullptr, 0, nullptr, nullptr);
      if (bytesNeeded > 0)
      {
        plHybridArray<char, 1024> dir;
        dir.SetCountUninitialized(bytesNeeded);
        WideCharToMultiByte(CP_UTF8, 0, directory.GetPtr(), directory.GetCount(), dir.GetData(), dir.GetCount(), nullptr, nullptr);

        Change& currentChange = ref_changes.ExpandAndGetRef();
        currentChange.eventFilePath = sDirectoryPath;
        currentChange.eventFilePath.AppendPath(plStringView(dir.GetData(), dir.GetCount()));
        currentChange.eventFilePath.MakeCleanPath();
        currentChange.Action = info->Action;
        currentChange.LastModificationTime = {};
        currentChange.isFile = true; // Pretend it's a file for now.
      }

      if (info->NextEntryOffset == 0)
      {
        break;
      }
      else
        info = (const FILE_NOTIFY_INFORMATION*)(((plUInt8*)info) + info->NextEntryOffset);
    }
  }

  void PostProcessNonNTFSChanges(plDynamicArray<Change>& ref_changes, plFileSystemMirrorType* pMirror)
  {
    plHybridArray<plInt32, 4> nextOp;
    // Figure what changes belong to the same object by creating a linked list of changes. This part is tricky as we basically have to handle all the oddities that plDirectoryWatcher::EnumerateChanges already does again to figure out which operations belong to the same object.
    {
      plMap<plStringView, plUInt32> lastChangeAtPath;
      nextOp.SetCount(ref_changes.GetCount(), -1);

      plInt32 pendingRemoveOrRename = -1;
      plInt32 lastMoveFrom = -1;

      for (plUInt32 i = 0; i < ref_changes.GetCount(); i++)
      {
        const auto& currentChange = ref_changes[i];
        if (pendingRemoveOrRename != -1 && (currentChange.Action == FILE_ACTION_RENAMED_OLD_NAME) && (ref_changes[pendingRemoveOrRename].eventFilePath == currentChange.eventFilePath))
        {
          // This is the bogus removed event because we changed the casing of a file / directory, ignore.
          lastChangeAtPath.Insert(ref_changes[pendingRemoveOrRename].eventFilePath, pendingRemoveOrRename);
          pendingRemoveOrRename = -1;
        }

        if (pendingRemoveOrRename != -1)
        {
          // An actual remove: Stop tracking the change.
          lastChangeAtPath.Remove(ref_changes[pendingRemoveOrRename].eventFilePath);
          pendingRemoveOrRename = -1;
        }

        plUInt32* uiUniqueItemIndex = nullptr;
        switch (currentChange.Action)
        {
          case FILE_ACTION_ADDED:
            lastChangeAtPath.Insert(currentChange.eventFilePath, i);
            break;
          case FILE_ACTION_REMOVED:
            if (lastChangeAtPath.TryGetValue(currentChange.eventFilePath, uiUniqueItemIndex))
            {
              nextOp[*uiUniqueItemIndex] = i;
              *uiUniqueItemIndex = i;
            }
            pendingRemoveOrRename = i;
            break;
          case FILE_ACTION_MODIFIED:
            if (lastChangeAtPath.TryGetValue(currentChange.eventFilePath, uiUniqueItemIndex))
            {
              nextOp[*uiUniqueItemIndex] = i;
              *uiUniqueItemIndex = i;
            }
            else
            {
              lastChangeAtPath[currentChange.eventFilePath] = i;
            }
            break;
          case FILE_ACTION_RENAMED_OLD_NAME:
            if (lastChangeAtPath.TryGetValue(currentChange.eventFilePath, uiUniqueItemIndex))
            {
              nextOp[*uiUniqueItemIndex] = i;
              *uiUniqueItemIndex = i;
            }
            else
            {
              lastChangeAtPath[currentChange.eventFilePath] = i;
            }
            lastMoveFrom = i;
            break;
          case FILE_ACTION_RENAMED_NEW_NAME:
            PL_ASSERT_DEBUG(lastMoveFrom != -1, "last move from should be present when encountering FILE_ACTION_RENAMED_NEW_NAME");
            nextOp[lastMoveFrom] = i;
            lastChangeAtPath.Remove(ref_changes[lastMoveFrom].eventFilePath);
            lastChangeAtPath.Insert(currentChange.eventFilePath, i);
            break;
        }
      }
    }

    // Anything that is chained via the nextOp linked list must be given the same type.
    // Instead of building arrays of arrays, we create a bit field of all changes and then flatten the linked list at the first set bit. While iterating we remove everything we reached via the linked list so on the next call to get the first bit we will find another object that needs processing. As the operations are ordered, the first bit will always point to the very first operation of an object (nextOp can never point to a previous element).
    plHybridBitfield<128> pendingChanges;
    pendingChanges.SetCount(ref_changes.GetCount(), true);

    // Get start of first object.
    plHybridArray<Change*, 4> objectChanges;
    auto it = pendingChanges.GetIterator();
    while (it.IsValid())
    {
      // Flatten the changes for one object into a list for easier processing.
      {
        objectChanges.Clear();
        plUInt32 currentIndex = it.Value();
        objectChanges.PushBack(&ref_changes[currentIndex]);
        pendingChanges.ClearBit(currentIndex);
        while (nextOp[currentIndex] != -1)
        {
          currentIndex = nextOp[currentIndex];
          pendingChanges.ClearBit(currentIndex);
          objectChanges.PushBack(&ref_changes[currentIndex]);
        }
      }

      // Figure out what type the object is. There is no correct way of doing this, which is the reason why ReadDirectoryChangesExW exists. There are however some good heuristics we can use:
      // 1. If the change is on an existing object, we should know it's type from the mirror. This is 100% correct.
      // 2. If object still exists, we can query its stats on disk to determine its type. In rare cases, this can be wrong but you would need to create a case where a file is replaced with a folder of the same name or something.
      // 3. If the object was created and deleted in the same enumeration, we cannot know what type it was, so we take a guess.
      {
        bool isFile = true;
        plFileSystemMirrorType::Type type;
        if (pMirror->GetType(objectChanges[0]->eventFilePath, type).Succeeded())
        {
          isFile = type == plFileSystemMirrorType::Type::File;
        }
        else
        {
          bool typeFound = false;
          for (Change* currentChange : objectChanges)
          {
            plFileStats stats;
            if (plOSFile::GetFileStats(currentChange->eventFilePath, stats).Succeeded())
            {
              isFile = !stats.m_bIsDirectory;
              typeFound = true;
              break;
            }
          }
          if (!typeFound)
          {
            // No stats and no entry in mirror: It's guessing time!
            isFile = objectChanges[0]->eventFilePath.FindSubString(".") != nullptr;
          }
        }

        // Apply type to all objects in the chain.
        for (Change* currentChange : objectChanges)
        {
          currentChange->isFile = isFile;
        }
      }

      // Find start of next object.
      it = pendingChanges.GetIterator();
    }
  }

} // namespace

struct plDirectoryWatcherImpl
{
  void DoRead();
  void EnumerateChangesImpl(plStringView sDirectoryPath, plTime waitUpTo, const plDelegate<void(const Change&)>& callback);

  bool m_bNTFS = false;
  HANDLE m_directoryHandle;
  DWORD m_filter;
  OVERLAPPED m_overlapped;
  HANDLE m_overlappedEvent;
  plDynamicArray<plUInt8> m_buffer;
  plBitflags<plDirectoryWatcher::Watch> m_whatToWatch;
  plUniquePtr<plFileSystemMirrorType> m_mirror; // store the last modification timestamp alongside each file
};

plDirectoryWatcher::plDirectoryWatcher()
  : m_pImpl(PL_DEFAULT_NEW(plDirectoryWatcherImpl))
{
  m_pImpl->m_buffer.SetCountUninitialized(1024 * 1024);
}

plResult plDirectoryWatcher::OpenDirectory(plStringView sAbsolutePath, plBitflags<Watch> whatToWatch)
{
  m_pImpl->m_bNTFS = false;
  {
    // Get drive root:
    plStringBuilder sTemp = sAbsolutePath;
    sTemp.MakeCleanPath();
    const char* szFirst = sTemp.FindSubString("/");
    PL_ASSERT_DEV(szFirst != nullptr, "The path '{}' is not absolute", sTemp);
    plStringView sRoot = sAbsolutePath.GetSubString(0, static_cast<plUInt32>(szFirst - sTemp.GetData()) + 1);

    WCHAR szFileSystemName[8];
    BOOL res = GetVolumeInformationW(plStringWChar(sRoot),
      nullptr,
      0,
      nullptr,
      nullptr,
      nullptr,
      szFileSystemName,
      PL_ARRAY_SIZE(szFileSystemName));
    m_pImpl->m_bNTFS = res == TRUE && plStringUtf8(szFileSystemName).GetView() == "NTFS" && !cvar_ForceNonNTFS.GetValue();
  }

  PL_ASSERT_DEV(m_sDirectoryPath.IsEmpty(), "Directory already open, call CloseDirectory first!");
  plStringBuilder sPath(sAbsolutePath);
  sPath.MakeCleanPath();
  sPath.Trim("/");

  m_pImpl->m_whatToWatch = whatToWatch;
  m_pImpl->m_filter = FILE_NOTIFY_CHANGE_FILE_NAME;
  const bool bRequiresMirror = whatToWatch.IsSet(Watch::Writes) || whatToWatch.AreAllSet(Watch::Deletes | Watch::Subdirectories);
  if (bRequiresMirror)
  {
    m_pImpl->m_filter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
  }

  if (!m_pImpl->m_bNTFS || bRequiresMirror)
  {
    m_pImpl->m_mirror = PL_DEFAULT_NEW(plFileSystemMirrorType);
    m_pImpl->m_mirror->AddDirectory(sPath).AssertSuccess();
  }

  if (whatToWatch.IsAnySet(Watch::Deletes | Watch::Creates | Watch::Renames))
  {
    m_pImpl->m_filter |= FILE_NOTIFY_CHANGE_DIR_NAME;
  }

  m_pImpl->m_directoryHandle = CreateFileW(plDosDevicePath(sPath), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
  if (m_pImpl->m_directoryHandle == INVALID_HANDLE_VALUE)
  {
    return PL_FAILURE;
  }

  m_pImpl->m_overlappedEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
  if (m_pImpl->m_overlappedEvent == INVALID_HANDLE_VALUE)
  {
    return PL_FAILURE;
  }

  m_pImpl->DoRead();
  m_sDirectoryPath = sPath;

  return PL_SUCCESS;
}

void plDirectoryWatcher::CloseDirectory()
{
  if (!m_sDirectoryPath.IsEmpty())
  {
    CancelIo(m_pImpl->m_directoryHandle);
    CloseHandle(m_pImpl->m_overlappedEvent);
    CloseHandle(m_pImpl->m_directoryHandle);
    m_sDirectoryPath.Clear();
  }
}

plDirectoryWatcher::~plDirectoryWatcher()
{
  CloseDirectory();
  PL_DEFAULT_DELETE(m_pImpl);
}

void plDirectoryWatcherImpl::DoRead()
{
  ResetEvent(m_overlappedEvent);
  memset(&m_overlapped, 0, sizeof(m_overlapped));
  m_overlapped.hEvent = m_overlappedEvent;

  if (m_bNTFS)
  {
    BOOL success =
      ReadDirectoryChangesExW(m_directoryHandle, m_buffer.GetData(), m_buffer.GetCount(), m_whatToWatch.IsSet(plDirectoryWatcher::Watch::Subdirectories), m_filter, nullptr, &m_overlapped, nullptr, ReadDirectoryNotifyExtendedInformation);
    PL_ASSERT_DEV(success, "ReadDirectoryChangesExW failed.");
    PL_IGNORE_UNUSED(success);
  }
  else
  {
    BOOL success =
      ReadDirectoryChangesW(m_directoryHandle, m_buffer.GetData(), m_buffer.GetCount(), m_whatToWatch.IsSet(plDirectoryWatcher::Watch::Subdirectories), m_filter, nullptr, &m_overlapped, nullptr);
    PL_ASSERT_DEV(success, "ReadDirectoryChangesW failed.");
    PL_IGNORE_UNUSED(success);
  }
}

void plDirectoryWatcherImpl::EnumerateChangesImpl(plStringView sDirectoryPath, plTime waitUpTo, const plDelegate<void(const Change&)>& callback)
{
  plHybridArray<Change, 6> changes;

  plHybridArray<plUInt8, 4096> buffer;
  while (WaitForSingleObject(m_overlappedEvent, static_cast<DWORD>(waitUpTo.GetMilliseconds())) == WAIT_OBJECT_0)
  {
    waitUpTo = plTime::MakeZero(); // only wait on the first call to GetQueuedCompletionStatus

    DWORD numberOfBytes = 0;
    GetOverlappedResult(m_directoryHandle, &m_overlapped, &numberOfBytes, FALSE);

    // Copy the buffer
    buffer.SetCountUninitialized(numberOfBytes);
    buffer.GetArrayPtr().CopyFrom(m_buffer.GetArrayPtr().GetSubArray(0, numberOfBytes));

    // Reissue the read request
    DoRead();

    if (numberOfBytes == 0)
    {
      return;
    }

    // We can fire NTFS events right away as they don't need post processing which prevents us from resizing the changes array unnecessarily.
    if (m_bNTFS)
    {
      GetChangesNTFS(sDirectoryPath, buffer, changes);
      for (const Change& change : changes)
      {
        callback(change);
      }
      changes.Clear();
    }
    else
    {
      GetChangesNonNTFS(sDirectoryPath, buffer, changes);
    }
  }

  // Non-NTFS changes need to be collected and processed in one go to be able to reconstruct the type of the change.
  if (!m_bNTFS)
  {
    PostProcessNonNTFSChanges(changes, m_mirror.Borrow());
    for (const Change& change : changes)
    {
      callback(change);
    }
  }
}

void plDirectoryWatcher::EnumerateChanges(EnumerateChangesFunction func, plTime waitUpTo)
{
  plFileSystemMirrorType* mirror = m_pImpl->m_mirror.Borrow();
  PL_ASSERT_DEV(!m_sDirectoryPath.IsEmpty(), "No directory opened!");

  MoveEvent pendingRemoveOrRename;
  const plBitflags<plDirectoryWatcher::Watch> whatToWatch = m_pImpl->m_whatToWatch;
  // Renaming a file to the same filename with different casing triggers the events REMOVED (old casing) -> RENAMED_OLD_NAME -> _RENAMED_NEW_NAME.
  // Thus, we need to cache every remove event to make sure the very next event is not a rename of the exact same file.
  auto FirePendingRemove = [&]()
  {
    if (!pendingRemoveOrRename.IsEmpty())
    {
      if (pendingRemoveOrRename.isDirectory)
      {
        if (whatToWatch.IsSet(Watch::Deletes))
        {
          if (mirror && whatToWatch.IsSet(Watch::Subdirectories))
          {
            mirror->Enumerate(pendingRemoveOrRename.path, [&](const plStringBuilder& sPath, plFileSystemMirrorType::Type type)
                    { func(sPath, plDirectoryWatcherAction::Removed, (type == plFileSystemMirrorType::Type::File) ? plDirectoryWatcherType::File : plDirectoryWatcherType::Directory); })
              .AssertSuccess();
          }
          func(pendingRemoveOrRename.path, plDirectoryWatcherAction::Removed, plDirectoryWatcherType::Directory);
        }
        if (mirror)
        {
          mirror->RemoveDirectory(pendingRemoveOrRename.path).AssertSuccess();
        }
      }
      else
      {
        if (mirror)
        {
          mirror->RemoveFile(pendingRemoveOrRename.path).AssertSuccess();
        }
        if (whatToWatch.IsSet(plDirectoryWatcher::Watch::Deletes))
        {
          func(pendingRemoveOrRename.path, plDirectoryWatcherAction::Removed, plDirectoryWatcherType::File);
        }
      }
      pendingRemoveOrRename.Clear();
    }
  };

  PL_SCOPE_EXIT(FirePendingRemove());

  MoveEvent lastMoveFrom;

  // Process the messages
  m_pImpl->EnumerateChangesImpl(m_sDirectoryPath, waitUpTo, [&](const Change& info)
    {
      plDirectoryWatcherAction action = plDirectoryWatcherAction::None;
      bool fireEvent = false;

      if (!pendingRemoveOrRename.IsEmpty() && info.isFile == !pendingRemoveOrRename.isDirectory && info.Action == FILE_ACTION_RENAMED_OLD_NAME && pendingRemoveOrRename.path == info.eventFilePath)
      {
        // This is the bogus removed event because we changed the casing of a file / directory, ignore.
        pendingRemoveOrRename.Clear();
      }
      FirePendingRemove();

      if (info.isFile)
      {
        switch (info.Action)
        {
          case FILE_ACTION_ADDED:
            DEBUG_LOG("FILE_ACTION_ADDED {} ({})", eventFilePath, info.LastModificationTime.QuadPart);
            action = plDirectoryWatcherAction::Added;
            fireEvent = whatToWatch.IsSet(plDirectoryWatcher::Watch::Creates);
            if (mirror)
            {
              bool fileAlreadyExists = false;
              mirror->AddFile(info.eventFilePath, true, &fileAlreadyExists, nullptr).AssertSuccess();
              if (fileAlreadyExists)
              {
                fireEvent = false;
              }
            }
            break;
          case FILE_ACTION_REMOVED:
            DEBUG_LOG("FILE_ACTION_REMOVED {} ({})", eventFilePath, info.LastModificationTime.QuadPart);
            action = plDirectoryWatcherAction::Removed;
            fireEvent = false;
            pendingRemoveOrRename = {info.eventFilePath, false};
            break;
          case FILE_ACTION_MODIFIED:
          {
            DEBUG_LOG("FILE_ACTION_MODIFIED {} ({})", eventFilePath, info.LastModificationTime.QuadPart);
            action = plDirectoryWatcherAction::Modified;
            fireEvent = whatToWatch.IsAnySet(plDirectoryWatcher::Watch::Writes);
            bool fileAreadyKnown = false;
            bool addPending = false;
            if (mirror)
            {
              mirror->AddFile(info.eventFilePath, false, &fileAreadyKnown, &addPending).AssertSuccess();
            }
            if (fileAreadyKnown && addPending)
            {
              fireEvent = false;
            }
          }
          break;
          case FILE_ACTION_RENAMED_OLD_NAME:
            DEBUG_LOG("FILE_ACTION_RENAMED_OLD_NAME {} ({})", eventFilePath, info.LastModificationTime.QuadPart);
            PL_ASSERT_DEV(lastMoveFrom.IsEmpty(), "there should be no pending move from");
            action = plDirectoryWatcherAction::RenamedOldName;
            fireEvent = whatToWatch.IsAnySet(plDirectoryWatcher::Watch::Renames);
            PL_ASSERT_DEV(lastMoveFrom.IsEmpty(), "there should be no pending last move from");
            lastMoveFrom = {info.eventFilePath, false};
            break;
          case FILE_ACTION_RENAMED_NEW_NAME:
            DEBUG_LOG("FILE_ACTION_RENAMED_NEW_NAME {} ({})", eventFilePath, info.LastModificationTime.QuadPart);
            action = plDirectoryWatcherAction::RenamedNewName;
            fireEvent = whatToWatch.IsAnySet(plDirectoryWatcher::Watch::Renames);
            PL_ASSERT_DEV(!lastMoveFrom.IsEmpty() && !lastMoveFrom.isDirectory, "last move from doesn't match");
            if (mirror)
            {
              mirror->RemoveFile(lastMoveFrom.path).AssertSuccess();
              mirror->AddFile(info.eventFilePath, false, nullptr, nullptr).AssertSuccess();
            }
            lastMoveFrom.Clear();
            break;
        }

        if (fireEvent)
        {
          func(info.eventFilePath, action, plDirectoryWatcherType::File);
        }
      }
      else
      {
        switch (info.Action)
        {
          case FILE_ACTION_ADDED:
          {
            DEBUG_LOG("DIR_ACTION_ADDED {}", eventFilePath);
            bool directoryAlreadyKnown = false;
            if (mirror)
            {
              mirror->AddDirectory(info.eventFilePath, &directoryAlreadyKnown).AssertSuccess();
            }

            if (whatToWatch.IsSet(Watch::Creates) && !directoryAlreadyKnown)
            {
              func(info.eventFilePath, plDirectoryWatcherAction::Added, plDirectoryWatcherType::Directory);
            }

            // Whenever we add a directory we might be "to late" to see changes inside it.
            // So iterate the file system and make sure we track all files / subdirectories
            plFileSystemIterator subdirIt;

            subdirIt.StartSearch(info.eventFilePath.GetData(),
              whatToWatch.IsSet(plDirectoryWatcher::Watch::Subdirectories)
                ? plFileSystemIteratorFlags::ReportFilesAndFoldersRecursive
                : plFileSystemIteratorFlags::ReportFiles);

            plStringBuilder tmpPath2;
            for (; subdirIt.IsValid(); subdirIt.Next())
            {
              const plFileStats& stats = subdirIt.GetStats();
              stats.GetFullPath(tmpPath2);
              if (stats.m_bIsDirectory)
              {
                directoryAlreadyKnown = false;
                if (mirror)
                {
                  mirror->AddDirectory(tmpPath2, &directoryAlreadyKnown).AssertSuccess();
                }
                if (whatToWatch.IsSet(plDirectoryWatcher::Watch::Creates) && !directoryAlreadyKnown)
                {
                  func(tmpPath2, plDirectoryWatcherAction::Added, plDirectoryWatcherType::Directory);
                }
              }
              else
              {
                bool fileExistsAlready = false;
                if (mirror)
                {
                  mirror->AddFile(tmpPath2, false, &fileExistsAlready, nullptr).AssertSuccess();
                }
                if (whatToWatch.IsSet(plDirectoryWatcher::Watch::Creates) && !fileExistsAlready)
                {
                  func(tmpPath2, plDirectoryWatcherAction::Added, plDirectoryWatcherType::File);
                }
              }
            }
          }
          break;
          case FILE_ACTION_REMOVED:
            DEBUG_LOG("DIR_ACTION_REMOVED {}", eventFilePath);
            pendingRemoveOrRename = {info.eventFilePath, true};
            break;
          case FILE_ACTION_RENAMED_OLD_NAME:
            DEBUG_LOG("DIR_ACTION_OLD_NAME {}", eventFilePath);
            PL_ASSERT_DEV(lastMoveFrom.IsEmpty(), "there should be no pending move from");
            lastMoveFrom = {info.eventFilePath, true};
            break;
          case FILE_ACTION_RENAMED_NEW_NAME:
            DEBUG_LOG("DIR_ACTION_NEW_NAME {}", eventFilePath);
            PL_ASSERT_DEV(!lastMoveFrom.IsEmpty(), "rename old name and rename new name should always appear in pairs");
            if (mirror)
            {
              mirror->MoveDirectory(lastMoveFrom.path, info.eventFilePath).AssertSuccess();
            }
            if (whatToWatch.IsSet(Watch::Renames))
            {
              func(lastMoveFrom.path, plDirectoryWatcherAction::RenamedOldName, plDirectoryWatcherType::Directory);
              func(info.eventFilePath, plDirectoryWatcherAction::RenamedNewName, plDirectoryWatcherType::Directory);
            }
            lastMoveFrom.Clear();
            break;
          default:
            break;
        }
      } //
    });
}


void plDirectoryWatcher::EnumerateChanges(plArrayPtr<plDirectoryWatcher*> watchers, EnumerateChangesFunction func, plTime waitUpTo)
{
  plHybridArray<HANDLE, 16> events;
  events.SetCount(watchers.GetCount());

  for (plUInt32 i = 0; i < watchers.GetCount(); ++i)
  {
    events[i] = watchers[i]->m_pImpl->m_overlappedEvent;
  }

  // Wait for any of the watchers to have some data ready
  if (WaitForMultipleObjects(events.GetCount(), events.GetData(), FALSE, static_cast<DWORD>(waitUpTo.GetMilliseconds())) == WAIT_TIMEOUT)
  {
    return;
  }

  // Iterate all of them to make sure we report all changes up to this point.
  for (plDirectoryWatcher* watcher : watchers)
  {
    watcher->EnumerateChanges(func);
  }
}

#endif

PL_STATICLINK_FILE(Foundation, Foundation_Platform_Win_DirectoryWatcher_Win);