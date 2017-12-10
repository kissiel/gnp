#include <git2.h>
#include <stdbool.h>
#include <stdio.h>
void init();
void shutdown();

int status_cb(const char *, unsigned int, void *);
int stash_cb(size_t, const char*, const git_oid *, void *);
typedef struct {
    int modified;
    int staged;
} status_data;


int main() {
    init();
    git_repository *repo = NULL;
    git_buf root = {0};
    int error = git_repository_discover(&root, ".", 0, NULL);

    if (error == GIT_ENOTFOUND) {
        // not inside a git repo
        printf("Not a repo\n");
        return 0;
    }
    char* path = root.ptr;
    printf("Opening: %s\n", path);

    error = git_repository_open(&repo, path);
    git_buf_free(&root);
    if (error < 0) {
        const git_error *e = giterr_last();
        printf("Error %d/%d: %s\n", error, e->klass, e->message);
        exit(error);
    }
    printf("%d\n", error);

    status_data d = {0};
    error = git_status_foreach(repo, status_cb, &d);
    if (error < 0) {
        const git_error *e = giterr_last();
        printf("Error %d/%d: %s\n", error, e->klass, e->message);
        exit(error);
    }
    printf("Staged: %i, Modified: %i\n", d.staged, d.modified);
    error = git_stash_foreach(repo, stash_cb, &d);
    if (error < 0) {
        const git_error *e = giterr_last();
        printf("Error %d/%d: %s\n", error, e->klass, e->message);
        exit(error);
    }
    /*
    printf("%d\n", error);
    git_reference *ref = NULL;
    error = git_reference_dwim(&ref, repo, "HEAD");
    const char* buf = NULL;
    git_branch_name(&buf, ref);
    printf("%s\n", buf);*/


    shutdown();
}

#if LIBGIT2_VER_MAJOR != 0
#error LIBGIT2_VER_MAJOR is not 0
#endif

void init() {
#if LIBGIT2_VER_MINOR > 20
    git_libgit2_init();
#else
    git_threads_init();
#endif

}

void shutdown() {
#if LIBGIT2_VER_MINOR > 20
    git_libgit2_shutdown();
#else
    git_threads_shutdown();
#endif
}

int status_cb(const char *path, unsigned int status_flags, void *payload)
{
        status_data *d = (status_data*)payload;
        // any of the following flags means the file is staged for commit
        int staged_flags = GIT_STATUS_INDEX_NEW | GIT_STATUS_INDEX_MODIFIED |
            GIT_STATUS_INDEX_DELETED | GIT_STATUS_INDEX_RENAMED |
            GIT_STATUS_INDEX_TYPECHANGE;

        if (status_flags & staged_flags) {
            d->staged++;
        }

        int modified_flags = GIT_STATUS_WT_NEW  | GIT_STATUS_WT_MODIFIED |
            GIT_STATUS_WT_DELETED | GIT_STATUS_WT_RENAMED |
            GIT_STATUS_WT_TYPECHANGE;

        if (status_flags & modified_flags) {
            d->modified++;
        }
        //printf("status_cb, path: %s\n flags: %u\n", path, status_flags);
        return 0;
}

int stash_cb( size_t index, const char* message, 
              const git_oid *stash_id, void *payload) {
    char pages[5] = {0xF0, 0x9F, 0x97, 0x90, 0};
    printf("%s index: %lu, message: %s\n", pages, index, message);
    return 0;

}
