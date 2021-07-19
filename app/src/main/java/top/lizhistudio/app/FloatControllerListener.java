package top.lizhistudio.app;

import android.util.Log;
import android.widget.Toast;

import com.immomo.mls.MLSEngine;
import com.immomo.mls.global.LVConfigBuilder;

import top.lizhistudio.androidlua.LuaContext;
import top.lizhistudio.app.core.ProjectManagerImplement;
import top.lizhistudio.app.view.FloatControllerView;
import top.lizhistudio.autolua.core.LuaInterpreter;
import top.lizhistudio.autolua.core.value.LuaValue;

public class FloatControllerListener implements FloatControllerView.OnClickListener {
    private final String projectName;
    public FloatControllerListener(String projectName)
    {
        this.projectName= projectName;
    }

    @Override
    public void onClick(FloatControllerView floatControllerView, int state) {
        LuaInterpreter luaInterpreter = App.getApp().getAutoLuaEngine();
        if (luaInterpreter == null)
        {
            Toast.makeText(App.getApp(),"错误的脚本执行环境，无法执行脚本",Toast.LENGTH_LONG)
                    .show();
        } else if (state == FloatControllerView.EXECUTEING_STATE)
        {
            luaInterpreter.interrupt();
        }else if (state == FloatControllerView.STOPPED_STATE)
        {
            String projectPath = ProjectManagerImplement.getInstance().getProjectPath(projectName);
            if (projectPath == null)
                return;
            MLSEngine.setLVConfig(new LVConfigBuilder(App.getApp())
                    .setRootDir(projectPath)
                    .setImageDir(projectPath+"/image")
                    .setCacheDir(App.getApp().getCacheDir().getAbsolutePath())
                    .setGlobalResourceDir(projectPath+"/resource").build());
            luaInterpreter.destroyNowLuaContext();
            App.getApp().setRootPath(projectPath);
            floatControllerView.setState(FloatControllerView.EXECUTEING_STATE);
            luaInterpreter.executeFile(
                    projectPath + "/main.lua",
                    LuaContext.CODE_TYPE.TEXT_BINARY,
                    new LuaInterpreter.Callback() {

                        @Override
                        public void onCallback(LuaValue[] result) {
                            floatControllerView.setState(FloatControllerView.STOPPED_STATE);
                        }

                        @Override
                        public void onError(Throwable throwable) {
                            if (throwable != null)
                                Log.e("AutoLuaEngine","call error",throwable);
                            floatControllerView.setState(FloatControllerView.STOPPED_STATE);
                        }
                    });
        }
    }
}
