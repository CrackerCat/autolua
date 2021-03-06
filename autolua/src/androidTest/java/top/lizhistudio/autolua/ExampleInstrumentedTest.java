package top.lizhistudio.autolua;

import android.content.Context;
import android.util.Log;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;

import top.lizhistudio.androidlua.JavaObjectWrapperFactoryImplement;
import top.lizhistudio.androidlua.LuaContext;
import top.lizhistudio.androidlua.LuaContextImplement;
import top.lizhistudio.autolua.core.AutoLuaEngine;
import top.lizhistudio.autolua.core.LuaContextFactory;
import top.lizhistudio.autolua.core.Server;

import static org.junit.Assert.assertEquals;

/**
 * Instrumented test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
@RunWith(AndroidJUnit4.class)
public class ExampleInstrumentedTest {
    private static final String TAG = "LuaContextTest";
    @Test
    public void useAppContext() {
        // Context of the app under test.
        Context appContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
        assertEquals("top.lizhistudio.autolua.test", appContext.getPackageName());
    }

    public static class MyLuaContextFactory implements LuaContextFactory
    {
        @Override
        public LuaContext newInstance() {
            JavaObjectWrapperFactoryImplement.Builder builder = new JavaObjectWrapperFactoryImplement.Builder();
            builder.registerThrowable();
            return new LuaContextImplement(builder.build());
        }
    }

    @Test
    public void testAutoLuaContext() throws Throwable
    {

    }
}