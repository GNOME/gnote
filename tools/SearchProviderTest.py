import dbus


def create_note(rc, title, content):
    note = rc.CreateNamedNote(title)
    rc.DisplayNote(note)
    content = title + "\n\n" + content
    rc.SetNoteContents(note, content)
    return note

def create_test_notes(rc, notes):
    for i in range(10):
        title = "SearchTestNote" + str(i)
        content = "Search Provider Test (" + str(i) + ")"
        notes.append(create_note(rc, title, content))
    create_note(rc, "SearchResult", "If you see this, the tests have passed!")
    rc.DisplaySearch()


def delete_test_notes(rc, notes):
    for note in notes:
        rc.DeleteNote(note)


def test_search_provider(proxy, notes):
    results = proxy.GetInitialResultSet(["SearchTestNote"])
    assert len(results) == 10
    results = proxy.GetInitialResultSet(["SearchTestNote1", "SearchTestNote2", "SearchTestNote3", "SearchTestNoteNonExistent", ""])
    assert len(results) == 3
    subsearch_titles = ["SearchTestNote2", "SearchTestNote3"]
    subresults = proxy.GetSubsearchResultSet(results, subsearch_titles)
    assert len(subresults) == 2
    result_metas = proxy.GetResultMetas(subresults)
    assert len(result_metas) == len(subresults)
    for meta in result_metas:
        assert meta["name"] in subsearch_titles
    proxy.LaunchSearch(["test"], 123)  # this shouldn't do anything, but shouldn't crash as well
    results = proxy.GetInitialResultSet(["SearchResult"])
    proxy.ActivateResult(results[0], ["SearchResult"], 123)

def main():
    bus = dbus.SessionBus()
    gnote_remote_control = bus.get_object("org.gnome.Gnote", "/org/gnome/Gnote/RemoteControl")
    rc_proxy = dbus.Interface(gnote_remote_control, "org.gnome.Gnote.RemoteControl")
    gnote_search_provider = bus.get_object("org.gnome.Gnote", "/org/gnome/Gnote/SearchProvider")
    search_proxy = dbus.Interface(gnote_search_provider, "org.gnome.Shell.SearchProvider2")

    notes = []
    create_test_notes(rc_proxy, notes)
    try:
        test_search_provider(search_proxy, notes)
    finally:
        delete_test_notes(rc_proxy, notes)


if __name__ == "__main__":
    main()
