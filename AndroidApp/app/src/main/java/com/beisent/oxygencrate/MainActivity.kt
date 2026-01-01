package com.beisent.oxygencrate

import android.Manifest
import android.app.NativeActivity
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.OpenableColumns
import android.provider.Settings
import android.view.KeyEvent
import android.view.inputmethod.InputMethodManager
import java.io.File
import java.io.FileOutputStream
import java.util.concurrent.LinkedBlockingQueue

class MainActivity : NativeActivity() {

    private val unicodeCharacterQueue = LinkedBlockingQueue<Int>()
    private val pendingFileSelections = LinkedBlockingQueue<String>()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        ensureExternalStoragePermission()
    }

    private fun ensureExternalStoragePermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (!Environment.isExternalStorageManager()) {
                try {
                    val intent = Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION)
                    intent.data = Uri.parse("package:$packageName")
                    startActivity(intent)
                } catch (_: Exception) {
                    val intent = Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION)
                    startActivity(intent)
                }
            }
        } else {
            val permissions = arrayOf(
                Manifest.permission.WRITE_EXTERNAL_STORAGE,
                Manifest.permission.READ_EXTERNAL_STORAGE
            )
            val missing = permissions.filter {
                checkSelfPermission(it) != PackageManager.PERMISSION_GRANTED
            }
            if (missing.isNotEmpty()) {
                requestPermissions(missing.toTypedArray(), 1001)
            }
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == REQUEST_OPEN_FILE && resultCode == RESULT_OK) {
            val uri: Uri = data?.data ?: return
            val takeFlags = data.flags and (Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
            try {
                contentResolver.takePersistableUriPermission(uri, takeFlags)
            } catch (_: Exception) {
            }
            copyUriToEditorDirectory(uri)?.let { pendingFileSelections.offer(it) }
        }
    }

    fun showSoftInput() {
        runOnUiThread {
            val inputMethodManager = getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
            val view = window.decorView
            view.post {
                view.requestFocus()
                inputMethodManager.showSoftInput(view, InputMethodManager.SHOW_FORCED)
            }
        }
    }

    fun hideSoftInput() {
        runOnUiThread {
            val inputMethodManager = getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
            val token = window.decorView.windowToken
            inputMethodManager.hideSoftInputFromWindow(token, 0)
        }
    }

    override fun dispatchKeyEvent(event: KeyEvent): Boolean {
        if (event.action == KeyEvent.ACTION_DOWN) {
            val unicodeChar = event.getUnicodeChar(event.metaState)
            if (unicodeChar != 0) {
                unicodeCharacterQueue.offer(unicodeChar)
            }
        }
        return super.dispatchKeyEvent(event)
    }

    fun pollUnicodeChar(): Int {
        return unicodeCharacterQueue.poll() ?: 0
    }

    fun openFilePicker() {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "*/*"
        }
        startActivityForResult(intent, REQUEST_OPEN_FILE)
    }

    fun pollSelectedFile(): String {
        return pendingFileSelections.poll() ?: ""
    }

    private fun getEditorDirectory(): File {
        val root = Environment.getExternalStorageDirectory()
        return File(root, "Beisent/OxygenCrate")
    }

    private fun copyUriToEditorDirectory(uri: Uri): String? {
        return try {
            val directory = getEditorDirectory()
            if (!directory.exists()) {
                directory.mkdirs()
            }

            val fileName = queryDisplayName(uri)
            val destination = File(directory, fileName)

            contentResolver.openInputStream(uri)?.use { input ->
                FileOutputStream(destination).use { output ->
                    input.copyTo(output)
                }
            } ?: return null

            destination.absolutePath
        } catch (_: Exception) {
            null
        }
    }

    private fun queryDisplayName(uri: Uri): String {
        var name = "Imported_${System.currentTimeMillis()}"
        val cursor = contentResolver.query(uri, arrayOf(OpenableColumns.DISPLAY_NAME), null, null, null)
        cursor?.use {
            val index = it.getColumnIndex(OpenableColumns.DISPLAY_NAME)
            if (index >= 0 && it.moveToFirst()) {
                name = it.getString(index)
            }
        }
        return name
    }

    companion object {
        private const val REQUEST_OPEN_FILE = 2002
    }
}
