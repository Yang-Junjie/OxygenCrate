package com.beisent.oxygencrate

import android.app.NativeActivity
import android.content.Context
import android.os.Bundle
import android.view.KeyEvent
import android.view.inputmethod.InputMethodManager
import java.util.concurrent.LinkedBlockingQueue

class MainActivity : NativeActivity() {

    private val unicodeCharacterQueue = LinkedBlockingQueue<Int>()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
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
}
