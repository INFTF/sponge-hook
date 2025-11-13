# sponge-hook

### A comprehensive test hook for Hookstore, manifest parsing, ABI generation, blobparts, hook parameters, and otxn_param

`sponge-hook` is a **test-focused Xahau Hook** designed specifically for:

* Verifying **Hookstore manifests**
* Exercising **HookParameters** (hp_*)
* Exercising **otxn_param** values (tp_*)
* Exercising **Invoke Blob** with **multiple structured blobparts**
* Verifying **hook state storage** and **ABI generation**
* Providing a predictable “sponge” that **accepts many input types** (accounts, uints, text, userConfig, etc.)
* Testing manifest → transaction → hook → state round-trip validity

It **does not implement business logic**.
Its job is to:

1. **Read** hp_*, tp_* and Blob fields
2. **Trace** all input values (so you see them in hook execution logs)
3. **Store** them in HookState for testing Hookstore’s ABI + installer

---

## 🔍 What This Hook Tests

### ✔ Hook Parameters (`hp_*`)

Configured at installation via **SetHook**:

* `hp_admin` (AccountID)
* `hp_limit` (utf8 string)
* `hp_note` (utf8 string)

These values are:

* Read via `hook_param()`
* Always traced to the debug log
* **Not** stored (they already live in HookObject)

---

### ✔ otxn Parameters (`tp_*`)

Provided via **Invoke (HookParameters override)**:

* `tp_sender` (AccountID)
* `tp_count` (ascii/utf8 numeric text)
* `tp_label` (ascii/utf8 text)

These values are:

* Read via `otxn_param()`
* Traced
* **Stored in HookState**

---

### ✔ Structured Blob via `sfBlob`

The Invoke also includes a Blob split into **multiple blobparts**:

| Part | Type    | Description           |
| ---- | ------- | --------------------- |
| 1    | ASCII   | `"CONF"` – 4-byte tag |
| 2    | uint8   | Version byte          |
| 3    | account | 20-byte AccountID     |
| 4    | uint64  | Limit (big-endian)    |
| 5    | utf8    | Remaining note text   |

The hook:

* Reads `sfBlob`
* Splits these parts
* Validates lengths
* Traces them
* Stores each piece individually in HookState

---

## 📦 Stored Hook State Keys

| Key            | Type                 | Source           |
| -------------- | -------------------- | ---------------- |
| `tp_sender`    | AccountID (20 bytes) | From otxn_param  |
| `tp_count`     | utf8 string          | From otxn_param  |
| `tp_label`     | utf8 string          | From otxn_param  |
| `blob_tag`     | ASCII (4 bytes)      | From Blob part 1 |
| `blob_version` | uint8                | From Blob part 2 |
| `blob_account` | AccountID (20 bytes) | From Blob part 3 |
| `blob_limit`   | uint64               | From Blob part 4 |
| `blob_note`    | utf8                 | From Blob part 5 |

This makes it ideal for:

* ABI generator testing
* State inspection
* Blob and param parsing verification
* UI testing
* Install workflow testing

---

## 📥 Installation Flow

Hookstore will present the following steps:

### 1. SetHook (install `main`)

Defines:

* Namespace
* HookParameters hp_*
* Hook code (WASM)
* ABI

### 2. Invoke (configuration test)

Provides:

* tp_* parameters via HookParameters override
* Complex structured Blob
* Signature by installer

After signing, your hook executes once and populates all state fields.

---

## 🛠 Hook Logic Summary

The hook code:

* Allocates only small buffers sized to real XRPL data:

  * 20 bytes for AccountID
  * 128 for strings
  * 256 for blob processing (safe upper bound)
* Always logs everything via `TRACEVAR` / `TRACESTR`
* Validates each otxn_param exists
* Validates blob minimum length
* Extracts fields using `memcmp`, byte slicing
* Stores each field using `state_set()`
* Always calls `accept()`

No business logic. No conditional rejects.
It should **never rollback**, unless malformed Blob or missing params occur.

---

## 🧪 Testing Scenarios

### Hookstore / UI Tests:

* Multi-field userConfig forms
* AccountID encoding (address → accountID)
* uint64 BE encoding
* Blobpart UI visualization
* ABI rendering
* State inspection tools

### Developer Tests:

* Validate exact behavior of `otxn_param`
* Validate blob parsing
* Audit JSON manifest correctness
* Ensure predictable traces for debugging
* Test ability to reorder and reinvoke hook

---

## 📄 File Structure

```
sponge-hook/
│
├── src/
│   └── sponge.c
│
├── hookstore.manifest.json
│
├── assets/
│   └── banner.png
│   └── icon.png
│   └── logo.png
│
├── .hookstore/
│   └── config.json
│
└── README.md            ← this file
```

---

## 🧾 License

MIT

---

## ✉ Contact / Support

This is a test hook intended for Hookstore + ABI development.
Open issues or suggestions welcomed in the repository.

---
