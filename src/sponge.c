#include "hookapi.h"

/*
 * sponge.c
 *
 * Test hook for Hookstore / ABI generator.
 *
 * It does three things:
 *  1) Reads HookParameters (hp_admin, hp_limit, hp_note) and TRACEs them (no state).
 *  2) Reads otxn_parameters (tp_sender, tp_count, tp_label) and stores them in state.
 *  3) Reads Blob and splits it into multiple fields, storing each as state:
 *
 *     Blob layout (byte offsets):
 *       Note: otxn_field(sfBlob) returns a VarString with a 1-byte length prefix
 *       0      : length prefix (VarString, 1 byte)
 *       1..4   : tag (ASCII "CONF", 4 bytes)
 *       5      : version (uint8)
 *       6..25  : account id (20 bytes)
 *       26..33 : limit (uint64, big-endian)
 *       34..N  : note (UTF-8, remaining bytes, optional)
 *
 *     Stored state keys:
 *       "tp_sender"    -> raw otxn_param bytes
 *       "tp_count"     -> raw otxn_param bytes
 *       "tp_label"     -> raw otxn_param bytes
 *
 *       "blob_tag"     -> 4 bytes
 *       "blob_version" -> 1 byte
 *       "blob_account" -> 20 bytes
 *       "blob_limit"   -> 8 bytes
 *       "blob_note"    -> remaining bytes (if any)
 *
 * The hook never rolls back; it always accept()s.
 */

int64_t hook(uint32_t reserved)
{
    _g(1,1); // init hook api

    TRACESTR("param_sponge_blob: start");

    // ---------------------------------------------------------------
    // 0) HookParameters from SetHook (trace only, no state)
    // ---------------------------------------------------------------

    {
        uint8_t buf[32];
        int64_t len = hook_param(SBUF(buf), "hp_admin", 8);
        if (len > 0)
        {
            TRACESTR("param_sponge_blob: hp_admin found");
            TRACEVAR(len);
        }
        else
        {
            TRACESTR("param_sponge_blob: hp_admin not set");
        }
    }

    {
        uint8_t buf[32];
        int64_t len = hook_param(SBUF(buf), "hp_limit", 8);
        if (len > 0)
        {
            TRACESTR("param_sponge_blob: hp_limit found");
            TRACEVAR(len);
        }
        else
        {
            TRACESTR("param_sponge_blob: hp_limit not set");
        }
    }

    {
        uint8_t buf[64];
        int64_t len = hook_param(SBUF(buf), "hp_note", 7);
        if (len > 0)
        {
            TRACESTR("param_sponge_blob: hp_note found");
            TRACEVAR(len);
        }
        else
        {
            TRACESTR("param_sponge_blob: hp_note not set");
        }
    }

    // ---------------------------------------------------------------
    // 1) otxn_parameters (typically on Invoke) → state
    // ---------------------------------------------------------------

    // tp_sender: AccountID (20 bytes expected)
    {
        uint8_t buf[20];
        int64_t len = otxn_param(SBUF(buf), "tp_sender", 9);
        if (len > 0)
        {
            TRACESTR("param_sponge_blob: tp_sender found");
            TRACEVAR(len);

            if (len == 20)
            {
                uint8_t key[] = "tp_sender";
                state_set(buf, 20, key, sizeof(key) - 1);
            }
            else
            {
                TRACESTR("param_sponge_blob: tp_sender len != 20, ignored");
            }
        }
        else
        {
            TRACESTR("param_sponge_blob: tp_sender not set");
        }
    }

    // tp_count: numeric ascii (small)
    {
        uint8_t buf[32];
        int64_t len = otxn_param(SBUF(buf), "tp_count", 8);
        if (len > 0)
        {
            TRACESTR("param_sponge_blob: tp_count found");
            TRACEVAR(len);

            uint8_t key[] = "tp_count";
            state_set(buf, (uint32_t)len, key, sizeof(key) - 1);
        }
        else
        {
            TRACESTR("param_sponge_blob: tp_count not set");
        }
    }

    // tp_label: text (reasonably small)
    {
        uint8_t buf[96];
        int64_t len = otxn_param(SBUF(buf), "tp_label", 8);
        if (len > 0)
        {
            TRACESTR("param_sponge_blob: tp_label found");
            TRACEVAR(len);

            uint8_t key[] = "tp_label";
            state_set(buf, (uint32_t)len, key, sizeof(key) - 1);
        }
        else
        {
            TRACESTR("param_sponge_blob: tp_label not set");
        }
    }

    // ---------------------------------------------------------------
    // 2) Blob → split into parts
    // ---------------------------------------------------------------

    {
        // 1 length prefix + 4 tag + 1 ver + 20 acc + 8 limit + up to ~64 note
        uint8_t blob[1 + 4 + 1 + 20 + 8 + 64];
        int64_t len = otxn_field(SBUF(blob), sfBlob);

        if (len > 0)
        {
            TRACESTR("param_sponge_blob: blob present");
            TRACEVAR(len);

            // Skip the VarString length prefix (byte 0)
            // Tag is at bytes 1-4
            if (len >= 5)
            {
                uint8_t key_tag[] = "blob_tag";
                state_set(blob + 1, 4, key_tag, sizeof(key_tag) - 1);
            }

            // Version is at byte 5
            if (len >= 6)
            {
                uint8_t key_ver[] = "blob_version";
                state_set(blob + 5, 1, key_ver, sizeof(key_ver) - 1);
            }

            // Account is at bytes 6-25
            if (len >= 26)
            {
                uint8_t key_acc[] = "blob_account";
                state_set(blob + 6, 20, key_acc, sizeof(key_acc) - 1);
            }

            // Limit is at bytes 26-33
            if (len >= 34)
            {
                uint8_t key_lim[] = "blob_limit";
                state_set(blob + 26, 8, key_lim, sizeof(key_lim) - 1);
            }

            // Note is at bytes 34+
            if (len > 34)
            {
                uint32_t note_len = (uint32_t)(len - 34);
                uint8_t key_note[] = "blob_note";
                state_set(blob + 34, note_len, key_note, sizeof(key_note) - 1);
            }
        }
        else
        {
            TRACESTR("param_sponge_blob: no blob present");
        }
    }

    TRACESTR("param_sponge_blob: accept");

    accept(SBUF("param_sponge_blob: ok"), 0);
    return 0;
}