<template>
  <div class="page-container centered">
    <div class="glass-card" style="max-width: 480px; width: 100%;" v-loading="loading">
      <div class="glass-card-header">
        <h3>{{ isEdit ? '编辑车辆' : '添加车辆' }}</h3>
      </div>
      <div class="glass-card-body padded">
      <el-form
        ref="formRef"
        :model="form"
        :rules="rules"
        label-position="top"
      >
        <el-form-item label="车牌号" prop="plate_number">
          <el-input v-model="form.plate_number" placeholder="例如 ABC-1234" />
        </el-form-item>

        <div class="form-row">
          <el-form-item label="品牌" prop="brand" style="flex: 1">
            <el-input v-model="form.brand" placeholder="例如 丰田" />
          </el-form-item>
          <el-form-item label="型号" prop="model" style="flex: 1">
            <el-input v-model="form.model" placeholder="例如 凯美瑞" />
          </el-form-item>
        </div>

        <div class="form-row">
          <el-form-item label="颜色" prop="color" style="flex: 1">
            <el-input v-model="form.color" placeholder="例如 白色" />
          </el-form-item>
          <el-form-item label="年份" prop="year" style="flex: 1">
            <el-input-number v-model="form.year" :min="2000" :max="2030" style="width: 100%" />
          </el-form-item>
        </div>

        <div class="form-row">
          <el-form-item label="日租金 (CNY)" prop="daily_rate" style="flex: 1">
            <el-input-number v-model="form.daily_rate" :min="0" :precision="2" style="width: 100%" />
          </el-form-item>
          <el-form-item label="押金 (CNY)" prop="deposit_amount" style="flex: 1">
            <el-input-number v-model="form.deposit_amount" :min="0" :precision="2" style="width: 100%" />
          </el-form-item>
        </div>

        <el-form-item label="里程 (km)">
          <el-input-number v-model="form.mileage" :min="0" style="width: 100%" />
        </el-form-item>

        <el-form-item label="图片URL">
          <el-input v-model="form.image_url" placeholder="车辆图片URL" />
        </el-form-item>

        <el-form-item label="描述">
          <el-input v-model="form.description" type="textarea" :rows="3" placeholder="车辆描述" />
        </el-form-item>

        <el-form-item>
          <el-button type="primary" :loading="submitting" @click="handleSubmit">
            {{ isEdit ? '保存修改' : '添加车辆' }}
          </el-button>
          <el-button @click="$router.back()">取消</el-button>
        </el-form-item>
      </el-form>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, computed, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { createVehicle, updateVehicle, getVehicleDetail } from '@/api/vehicle'
import { ElMessage } from 'element-plus'

const route = useRoute()
const router = useRouter()
const isEdit = computed(() => !!route.params.id)
const loading = ref(false)
const submitting = ref(false)

const formRef = ref(null)
const form = reactive({
  plate_number: '',
  brand: '',
  model: '',
  color: '',
  year: 2024,
  mileage: 0,
  daily_rate: 0,
  deposit_amount: 0,
  image_url: '',
  description: '',
})

const rules = {
  plate_number: [{ required: true, message: '请输入车牌号', trigger: 'blur' }],
  brand: [{ required: true, message: '请输入品牌', trigger: 'blur' }],
  model: [{ required: true, message: '请输入型号', trigger: 'blur' }],
  color: [{ required: true, message: '请输入颜色', trigger: 'blur' }],
  daily_rate: [{ required: true, message: '请输入日租金', trigger: 'blur' }],
  deposit_amount: [{ required: true, message: '请输入押金金额', trigger: 'blur' }],
}

async function loadVehicle() {
  if (!isEdit.value) return
  loading.value = true
  try {
    const res = await getVehicleDetail({ id: route.params.id })
    if (res.error === 0) {
      const v = res.vehicle || res
      Object.assign(form, {
        plate_number: v.plate_number || '',
        brand: v.brand || '',
        model: v.model || '',
        color: v.color || '',
        year: v.year || 2024,
        mileage: v.mileage || 0,
        daily_rate: v.daily_rate || 0,
        deposit_amount: v.deposit_amount || 0,
        image_url: v.image_url || '',
        description: v.description || '',
      })
    }
  } catch {
    ElMessage.error('加载车辆信息失败')
  } finally {
    loading.value = false
  }
}

async function handleSubmit() {
  const valid = await formRef.value.validate().catch(() => false)
  if (!valid) return

  submitting.value = true
  try {
    const data = { ...form }
    if (isEdit.value) data.id = Number(route.params.id)
    const res = isEdit.value ? await updateVehicle(data) : await createVehicle(data)
    if (res.error === 0) {
      ElMessage.success(isEdit.value ? '车辆已更新' : '车辆已添加')
      router.push('/vehicles')
    } else {
      ElMessage.error('操作失败')
    }
  } catch {
    ElMessage.error('操作失败')
  } finally {
    submitting.value = false
  }
}

onMounted(loadVehicle)
</script>

<style scoped>
.centered {
  display: flex;
  justify-content: center;
  align-items: flex-start;
  padding-top: 80px;
}
.form-row {
  display: flex;
  gap: 20px;
}
</style>
